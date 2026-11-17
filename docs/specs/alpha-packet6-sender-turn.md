# Alpha packet 6 sender turns

**FINAL / STABLE / HELD**

Native-source contract for the original Alpha dedicated server and the matching
world-step consumer in the original client. The scope is well-formed interaction
records, the live actor and world state they read or change during a step, and
the resulting event order and random draws.

## Evidence and conventions

- **S** is `Server.exe`, SHA-256
  `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
- **C** is `Cube.exe`, SHA-256
  `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Addresses are RVAs from image base 0x400000. Evidence consists of
  disassembly and data from those PE images. The dispatcher data at S 0x1489e0
  and C 0x222c20 identifies all eight entries. The entire request-through-health
  region S 0x136259–0x137987 and C 0x210499–0x211bc7 has corresponding instruction
  boundaries, flow and field accesses, with image-specific addresses for data
  and helper calls. The Cube step is reached from C 0x8cf2e and 0x8cf49.
- The world byte at +0xb4 is called the online flag here. Authority in the
  request handlers means that byte is zero. The dedicated server constructs
  the world with it clear and with a null local-actor pointer at +0xb8
  (S 0x149c50, 0xc8570). The in-process authority path in Cube also has the
  flag clear and can have a local actor. A local-actor check is a comparison
  to the world pointer at +0xb8.
- Actor and world-item positions use signed 64-bit fixed-point components,
  65,536 units per block. The pickup and pet range helpers subtract in fixed
  point, convert the signed differences to binary32 block units, then sum
  the three squared components in binary32 (S 0x2c50, 0x2550, 0x21b0).
- The draw counts below refer to the interaction paths' MSVCR110 `rand`
  calls. S uses the import slot at 0x1582f4 and C uses 0x2fc468.
  The scaled draws use binary32, with native multiply before divide by
  32,767. They belong at their call sites within the world step. Ordinary
  actor simulation, generation and other subsystems have their own calls.

## Receipt and sender turn

The 300-byte record starts with a 280-byte item. Kinds 2, 3 and 5 use the
three signed 32-bit values at +0x118 for zone X, zone Y and target index.
The unsigned dispatch byte is at +0x128. This consumer does not use +0x124
through +0x127 or the last three bytes. It uses the embedded item for kinds
1 and 6. Kind 5 copies the live ground item. Kind 7 takes its selection from
the live actor.

S receives the payload in case 6 of S 0x26020. It enters the world critical
section at +0x8000c0, resolves the connection's signed 64-bit actor ID in
the live world registry, appends at creature +0x130c and leaves the critical
section (S 0x262c3–0x262e7, 0x5420, 0x1caf0, 0xd3df0, 0xd5c60).
There is no kind-based routing or interaction RNG on the receive path.
Packet 0 can update live creature data under the same world lock. The
queue does not capture the sender's position, action or health at receipt.
The dedicated main loop holds the same lock across the world step and
its event fan-out (S 0x14a081, 0x14a0ae, 0x14a0d0, 0x14a24a).

The world step has earlier passes before it reaches the per-actor sweep.
The early clock pass sees the step's starting resting actors, and the
incoming hit application and generated-creature removal and activation
pass boundaries precede this sweep (S 0x132340–0x1324a4, 0x132b5b,
0x133e10–0x1341d9, 0x13491c–0x135702, 0x135702–0x136259). In the early
pass, a controller-0 actor with action 84 and positive health accelerates
clock advance from ten times elapsed milliseconds to one hundred times
elapsed milliseconds. Only the local actor in that starting state receives
the associated health increase, adding maximum health times elapsed
milliseconds scaled by 0.001 and then 0.05, with the cap applied there.
After clock advance the native pass sets the time to 09:00 if the 32-bit
field at world +0xa4 is zero. Day rollover increments the day and
subtracts 86,400,000 ms while the time is strictly above that value. A
request that changes the action or clock later in this step does not
revisit that early pass.

The per-actor sweep enters at the current first key of the world actor
registry at world +4. The registry orders its keys as **signed 64-bit IDs**
in ascending order. The high word compares as signed and a high-word tie
compares the low word as unsigned. It skips a null actor value. It obtains
the successor from the current live registry node **after** the actor's
whole turn, including when a turn ends at the health gate. It has neither
a snapshot of IDs nor a successor captured before interaction. A key
inserted below the current key will not receive a turn in this sweep.
A surviving higher key is reached in its current order, and a deleted
higher key is absent. Registry rotations preserve the current surviving
node. See S 0x136259–0x1362ac, 0x145be1–0x145c21, 0x7a30, 0x7a50,
0x130600, 0x2b10 and C 0x210499, 0x21fe21, 0x68d70, 0x68ad0.

Each non-null actor processes its **mixed queue in append order**, with
each record run to completion before the next. This includes repeated
kinds, kind 4 and unknown bytes. A request reads all effects committed by
previous requests and previously visited actors. The loop walks entries
while they remain linked, advances past each entry regardless of outcome,
and clears the whole list after traversal only with the online flag clear.
A rejected authority request therefore leaves no queued retry in an
authority world. With the flag set, the world still visits the entries,
performs the permitted parts of kind 3 and leaves the queue linked. The
connected Cube sender separately copies and removes at most one front
record per sender iteration for transmission (C 0x69c10, 0x69db2–0x69e3b).
S 0x1362ac–0x136306, 0x137804–0x137856, 0x12ebf0, 0x26f60 and
C 0x2104ec–0x210546, 0x211a44–0x211a96 establish the traversal and clear.

After the queue, compute the actor's maximum health and reduce current
health at +0x16c to that maximum if greater. The original recalculates the
maximum for the store on that path. If resulting health is at or below zero,
advance immediately to the next actor. The queue itself has no health gate.
A registered actor at zero health can perform a valid request and a heal
can put that actor through the remainder of the turn. Several queued heals
accumulate before this cap. An admitted request can change a later actor
before the later actor reaches its own health gate.

For a positive-health actor, the nonplayer and authority path increments
the behavior accumulator at +0x13c4 by elapsed milliseconds. At 32 ms or
more and with hit stun at +0x12c at or below zero, it calls the behavior
object at +0x13e4 if present, then resets the accumulator. With positive
hit stun it skips the behavior call. Type 144 loses binary32 0.025 times
elapsed milliseconds in the authority world and its health floors at
zero. This subtraction is after the initial health gate and has no
immediate second health gate. The actor's remaining action, movement,
collision and other turn processing precedes successor selection. Dynamic
objects and projectiles follow the actor sweep. See S 0x137856–0x137987,
0xfda0, 0x145be1, 0x145c21 and C 0x211a96–0x211bc7, 0x44db0, 0x21fe21,
0x21fe61.

## Request effects and draw order

The authority flag gates kinds 1, 2, 5, 6, 7 and 8 before their work.
Kind 4 and out-of-range dispatch bytes advance without mutation or draws.
Kind 3 has its own gates below. Section numbers refer to the world-update
packet and identify event classes in the step's output collection.

### Kind 1 — use item

S 0x136e64 and C 0x2110a4 distinguish item type byte 1 with subtype byte 7
from every other item. There is no consumption or inventory-ownership check
in this branch of the world consumer.

For type 1 subtype 7, the new ID is one less than the least signed key
with a non-null actor, with zero included as a baseline. The fresh creature
is constructed for that ID and installed in the live table. It receives the
sender's current fixed-point position, the signed 16-bit item level from
item +0x10 extended to the creature level, type 144 and controller 6.
The fresh creature's initialized health is 1. Its appearance initializer
receives no appearance seed record and consumes **five** draws. The first
three supply values modulo 256 to the third, second and first seed byte,
in that order. The next two produce values modulo 100.
The type-144 appearance path uses the fixed model 0x9f2, dimensions
0.8 by 0.8 by 0.8, offset (0, 0, 1) in appearance field +0x64 and scale
0.8 at appearance +0x28. It consumes no additional draw and does not select
a variant from the item. The bomb request emits no direct sound, heal hit
or gift. Its key is below the requesting actor and the bomb's actor turn
starts in a later sweep, including its fuse subtraction. Bomb handling
at zero health belongs to an earlier pass in a subsequent authority step.
See S 0x136e84–0x136fcd, 0x6400, 0x7020, 0xa840, 0xa88b–0xa8ee,
0xef60, 0xe758–0xe7ba, 0x134974–0x1349a5 and C 0x21118d–0x211205,
0x3b690, 0x3f7c0, 0x3f80b–0x3f86c, 0x436f9.

For other items, add the healing helper's output to the actor's current
health immediately. The helper returns zero except on type 1 subtypes
1, 2, 4, 5, 6, 8 and 9. The first five use a multiplier of 200 and
the last two use 100. The multiplied scale is the product of two powers
of 2. The first exponent is three times the difference between 1 and the
reciprocal of 1 plus 0.05 times the difference between item level and 1.
The second exponent is 0.25 times the unsigned byte at item +0x0c.
The exponents are calculated in binary32, the powers in binary64, and
the power results are rounded to binary32 before the product and final
multiplier. Emit sound 44 at the current actor position, pitch and volume
1, then a section-2 hit with attacker ID zero, target the sender, amount
the sign-negated heal, kind byte zero and the same fixed-point position.
The events occur even for a zero heal. The hit is an output record and
the health change already occurred in this turn. **Zero draws.**
See S 0x136fd2–0x1370b2, 0x13be0, 0x10f00, 0x22a90,
C 0x211212–0x2112f2, 0xc6e10.

### Kind 2 — talk and inn rest

S 0x13630d and C 0x21054d start a separate live scan from the first actor
key. Ignore null actors and actors whose home triple at +0x1b0 does not
equal the request triple. For a match, class byte 132 at +0x140 and clock
time at or after 18:00 or at or before 06:00 take the inn path immediately.
Otherwise, bit 0x80 at creature +0x7e ends the scan with no effect for that
match. With that bit clear, continue to the live successor. There is no
sender distance check.

Inn rest increments the day at world +0x800160 and writes 25,200,000 ms
(07:00) to world +0x80015c. It appends section-4 sound 29 at the sender's
current position with pitch 1 and volume 0.75. Next it scans the live
registry for actors whose controller byte is neither 0 nor 5 and whose
home zone is loaded with an in-range spawn index. For each, set the
referenced spawn countdown at spawn +0x38 to zero, remember its key and
destroy the actor. Erase the remembered keys after that scan.
Controller-0 senders, controller-5 pets and actors without a qualifying
home survive. A bomb just inserted by an earlier request has home-zone
coordinates (−1, −1) and survives. A deleted actor ahead of the current
sender loses its turn in this sweep. The already queued output of a
deleted actor behind the sender survives. The world then clears the
mission-generation flag at +0x15a18 of every loaded world group.
Other requests in this queue and later turns see the restored clock and
the changed registry immediately. A second inn request in the same queue
sees 07:00, outside the rest interval. The step's activation pass is
already complete. **Zero draws** in the handler and cleanup.
See S 0x136350–0x136454, 0x137c0, 0xd7ae0, 0x7aa0, 0x71db, 0x124500
and C 0x210590–0x210694, 0x1a4980, 0x1f0c80.

### Kind 3 — static entity interaction

S 0x13670a and C 0x21094a first clear action 79 at creature +0x68 to zero
and zero the action counter at +0x1190 if that action was set. This happens
even when online or when the target fails. The common admission is a
loaded requested zone and a valid signed index in its static-entity
vector at zone +0x0c. There is no common range check. The live entity kind
and state determine the following paths.

**Kind 45.** Require the requesting actor to equal world +0xb8. The world
online flag does not gate this branch. Look up the zone implied by the
nearest-static-entity helper at the sender's current position, then use
the original kind-45 entity's signed link at +0x178 as index in that
zone's static entities. The helper scans the neighboring three-by-three
8-block spatial buckets, ascending bucket X and then Y. It compares
squared 3D distance by squaring in fixed point, dividing each square by
65,536 as an integer, summing and converting the sum to squared block
units. It keeps the first minimum within squared range 16. Its fallback triple is
(0, 0, −1). This caller uses only its zone X and Y. If the selected zone
and link index exist, emit sound 47 at the old sender position before
moving the sender to the linked entity's fixed-point position plus two
blocks of Z. This branch emits no section-6 record. It is unavailable to
dedicated players because the dedicated world has no local actor.
**Zero draws.** See S 0x1367a3–0x1368c9, 0xd9410 and C 0x2109e3,
0x1a5f60.

**Free seats and resting entities.** Kinds 16, 18, 19, 68 and 69 qualify
if their 64-bit occupant at entity +0x40 is zero and the sender's post-cancel
action is 0, 83 or 84. With authority, store the sender ID in the occupant.
Set sender position from the entity's fixed-point origin with zero X/Y
offset and a Z offset equal to the entity's binary32 value at +0x2c plus
half the sender's binary32 value at +0x88. Convert that offset to fixed
point before adding it. Zero the three components at creature +0x34.
For 16 and 18 set action 83 and heading, at creature +0x30, to 90 degrees
times the entity's signed orientation at +0x20 plus 180 degrees.
For 19, 68 and 69 set action 84 and heading to 90 degrees times that
orientation. The first heading is formed by multiplying the integer
orientation plus two by 90 before converting to binary32. Append a
section-6 snapshot of the requested entity after its occupant changes.
Finally zero the three components at creature +0x40. In an online world
only the latter zeroing runs for an admitted free seat. **Zero draws.**
See S 0x1368ce–0x136b98, 0x148a00–0x148a41 and C 0x210b0e,
0x222c40–0x222c81.

**Chest contents.** Otherwise require the authority flag clear. Entity
kind 10 in state 2 iterates its groups at entity +0x48 in vector order,
then each group's stored entries in list order. Repeat for each unit in a
nonzero count. For each unit, draw Y offset first and X offset second.
Each offset is 2 minus four times the draw divided by 32,767. Draw a
third value for the incoming drop angle in degrees, multiplying by 360
and dividing by 32,767. Add the X/Y offsets to the chest position
after conversion to fixed point, with zero Z offset, and invoke the
kind-6 drop helper with scale 1. Its timer draw is fourth and its
conditional replacement-angle draw is fifth. Decrement the entry
after the attempt, including when the drop zone is missing. On exhaustion
the count and the stored item's first two bytes are zeroed. Chest loot
uses the helper directly and does not emit the kind-6 sound. Proceed to
the ordinary trigger after these attempts, including for an empty chest.
See S 0x136ba3–0x136d94, 0x136c48, 0x136c77, 0x136ccc, 0x5550,
0xd2810 and C 0x210de3–0x210fd4, 0x19aeb0.

**Ordinary trigger.** In authority, kind 1, 2, 3, 9 or 10 sends desired
state 1 for old state zero and desired state 0 otherwise. The helper
changes state only when it differs from the requested value, sets the
entity's 32-bit timer at +0x34 to the greater of zero and 1,000 minus its
old timer, then emits a sound at the entity. The usual sound is 54.
Entity kind 5 uses 51 in either direction, kind 6 uses 53 for state 1
and is silent for state 0, and kind 7 uses 52 for state 1 and is silent
for state 0. Pitch and volume are 1. The caller appends section 6 after
the helper using the requested entity's resulting state. A kind-9
entity with an in-range link at +0x178 in the same zone then applies
its new zero/nonzero state to the linked entity through the same helper.
This can append a second sound. The caller appends a second section-6
snapshot for the original kind-9 entity and its original target triple.
It does not substitute the link's entity or index in that record.
**Zero draws** in the trigger and link helpers. See S 0x136d94–0x136e5f,
0xd8c90, 0x22d70, 0x22da0 and C 0x210fd4–0x21109f, 0x1a58e0.

### Kind 5 — pick up

S 0x136506 and C 0x210746 require a loaded requested zone, a nonnegative
index within that zone's current ordered item vector at +0x30, eligibility
for that item, and squared distance at most 16 from its current position.
The eligibility helper accepts every item type except 25 directly.
For type 25 it compares the item's signed 32-bit field at +4 to the
sender's signed 32-bit field at +0x1164. If the sender field is above 100,
take its remainder modulo 100 plus 100 before comparing. A failed gate
has no event, item mutation or draw. The request item is unused.

On success, mark the zone in the changed-item set and append the section-9
gift with the sender's 64-bit ID and the actual ground item. If the sender
is also the local actor and the item type is other than 25, run the local
inventory-add helper now. Next draw **once** for sound 45 pitch, multiplying
the draw by 0.1, dividing by 32,767 and adding 1. Append the sound at the
sender's current position, volume 1. Then erase this item from the live
vector, shifting following indices down. A later record using the same
index sees the shifted item or an out-of-range index. No broadcast of the
whole item vector occurs inside this branch. See S 0x136513–0x136705,
0x9660, 0x27000, 0xd4430, 0xd65d0 and C 0x210753–0x210945, 0x3e550,
0x6ebe0, 0x19c800.

### Kind 6 — drop and the shared drop helper

S 0x13778c and C 0x2119cc pass the record's item and the sender's current
position to the shared helper, with incoming angle zero and scale 1.
The caller appends sound 57, pitch and volume 1, at that sender position
when the helper returns, including when the helper skipped placement.
This world branch does not remove inventory or check for an existing
inventory entry.

The helper first builds a world-item record with scaled base motion
scalar 1/14, draws a timer of 500 plus the remainder modulo 300, and
records the current world day. It derives the zone from X and Y by
truncating the fixed-point coordinates to blocks, then dividing by 256
with truncation towards zero. If the zone is missing, return after the
timer draw. If it exists, copy the item and draw again for the angle,
multiplying by 360 then dividing by 32,767. This replaces the incoming
angle, including the angle drawn by the chest caller. The initial
vertical block is the fixed-point Z quotient truncated towards zero,
with an extra subtraction of 1 when the original fixed-point Z is
negative. Probe downward at the given X/Y until the block's low five
type bits are neither 0 nor 2. Store Z one block above that support,
preserve fixed-point X and Y, append to the live item vector and mark the
zone changed. The helper has **one draw for a missing zone and two for a
loaded zone**, in timer-then-angle order. See S 0x137799–0x1377f9,
0xd2810–0xd2ac8, 0x1d8d0, 0x1f5b0 and C 0x2119d9–0x211a39, 0x19aeb0.

### Kind 7 — voxel object

S 0x1370b7 and C 0x2112f7 look up the signed byte at actor +0x18d
in the live world's voxel-object definitions. Stop on missing definition.
The anchor starts from the current actor position. Convert the three
binary32 components at actor +0x160 to fixed point, add them and a
fixed-point half block on each axis, then truncate each resultant
coordinate to an integer block.
Use the signed byte at actor +0x18c in quarter-turns for orientation.
The horizontal model transform uses a 90-degree rotation per
quarter-turn and offsets of minus half the X and Y dimensions before
rotation. Model coordinates use voxel centers with a half-block
adjustment and the native truncation after transforming to world space.

Under the block-edit critical section at world +0x8000d8, visit model X
in ascending order, then Y, then Z. For each column first find the
extreme nonzero RGB voxels. All-zero RGB between those extremes is
written as an interior clear with byte 0x40 and has no direct section-1
record. Nonzero RGB checks the existing destination block's 0x40 bit and
skips protected cells. Otherwise write the RGB with type/flag byte 0x46
and append section 1 with that position, block data and the current world
day. The native writer handles the actual bounds and terrain column.
After voxel writes the dedicated path runs the additional region
propagation helper for its native footprint. The unsupported-prop
cleanup then runs before the object list append. If the anchor's zone is
loaded, append the object selection and anchor to that zone's live
16-byte object list, and mark it for section 8. Release the block-edit
lock. There is no kind-7 sound or gift. **Zero draws** on the traced
placement and finalization path. See S 0x1370ca–0x137787, 0xd1860,
0xd8e10, 0x2030, 0x23b0, 0x14f0, 0x21f0, 0xc64f0, 0x1ff00, 0xd1a70,
0xd9160, 0xd3e00, 0xd5c70 and C 0x21130a–0x211a3a, 0x198840,
0x1a5af0, 0x19a0e0, 0x1a5cb0.

### Kind 8 — reset pet and sender combat state

S 0x136459 and C 0x210699 resolve the sender's 64-bit pet ID at +0x11c8
in the current live actor registry. If found, clear the pet's threat map
at +0x13a4, then move it to the sender's current fixed-point position
only for squared distance strictly above 40,000. No further pet state or
velocity is copied by this branch. Even if the pet was absent, clear the
sender's hit-counter map at +0x13ac and write zero to its 64-bit target
at +0x11d0. It emits no direct event and makes **zero draws**. See S
0x136466–0x136501, 0xf8520, 0x5420 and C 0x2106a6–0x210741,
0x27e480, 0x2f000.

## Within-step visibility and outbound order

The next request uses the latest actor position, entity action and
occupancy, current live item indices, clock/day, and actor membership.
For example, a seat followed by a drop uses the seated position. A drop
following inn rest has the incremented day. A later sender cannot pick
up an item already erased by an earlier sender, though an item shifted
into that index may qualify. A later sender checks an earlier sender's
immediate seat claim. Earlier actors' movement between queues can change
the world a later sender sees.

Appends within each event class preserve their order. World items and
object lists change in their handlers. The step's wrap-up takes the full
current item vector for each changed zone into section 7, full current
object list for each changed object zone into section 8, marks touched
entity zones for saving and records section-1 edits in their loaded
zones (S 0x146efc–0x147222). A full list thus reflects all same-step
inserts and erasures. Section 6 snapshots are captured at the call sites
even if the same entity changes again before the step ends.

The dedicated receiver emits no packet-6 acknowledgement. The event
fan-out at S 0x272d0 copies the produced hit and gift lists to each
connection and filters sounds by a strict squared range below 40,000
from each connection actor's position at fan-out. Gifts carry recipient
IDs in the records. The normal per-connection writer samples the clock
and buffered events under the world lock, sends clock packet 5 with day
then time, and then sends world packet 4. Packet 4 serializes event classes
in section order, including block edits 1, hits 2, sounds 4, static
entities 6, item lists 7, object lists 8 and gifts 9. A sound appended
before a hit or after a gift describes simulation-side order. They retain
their section-specific order on the wire. This is an asynchronous
connection writer and a send can contain events accumulated from more
than one simulation step. See S 0x23dd0, 0x272d0, 0x21b10, 0x21b80,
0x21c60, 0x21d50, 0x21a30.

## Limits and corrections to the earlier outline

- The held S step-12 outline describes a health-gated remainder of the
  actor turn. The live queue runs before that gate, and the online
  flag gates the queue's destruction. S 0x1362ac–0x136306 and
  0x137804–0x1378a1 establish both. The earlier packet-6 wire table
  identifies receipt only. The PE dispatch tables identify all handlers.
- The held S section 8 summarizes static triggers as state-0 actions.
  S 0x136daf–0x136dc3 proves zero/nonzero toggling, including closing
  a state-2 chest after its loot attempts. S 0x136df2–0x136e5a shows
  the linked-entity side effect and the duplicate original switch
  snapshot. S 0x1367a3 and 0x136916–0x136b64 show the kind-3 local and
  online exceptions separately from authority mutations.
- The one draw at the pickup sound comes after all admission, and
  ordinary drops or chests do not have a single unconditional angle
  draw. For the latter the caller's angle is consumed and the helper can
  replace it. S 0x13667a, 0x136c48–0x136d10, 0xd288e, 0xd2902,
  0xd2919. The bomb's fixed appearance still consumes the five
  initializer draws at S 0xa88b–0xa8ee.
- This scope establishes the request effects, their normal native
  dependencies and draw order. Socket truncation and failure recovery,
  signed ID exhaustion, invalid world/object storage, all clipping and
  lighting internals of kind 7's terrain writer, behavior of unrelated
  actor actions and the full offline audio/inventory output consumer are
  unresolved for this contract. The same-step evidence concerns a
  surviving sender, including native controller-0 connection actors.
  No behavior is specified for a fabricated request on an actor that
  would destroy itself in the inn cleanup. Native-image inspection
  provides static proof within this scope. Runtime parity and acceptance
  remain open.
