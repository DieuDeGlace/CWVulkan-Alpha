# Alpha client interaction queue (Packet 6)

Status **FINAL / STABLE / HELD**

This contract covers the original Alpha producer and consumer paths, including
single-player use of the same request format. Evidence is static inspection of
the two original x86 PE images and their recovered listings. The hexadecimal
addresses prefixed C or S are image-relative virtual addresses in these images.

| Image | SHA-256 |
|---|---|
| C — `Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| S — `Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

## Request record and identity

The request is 300 bytes. The creature owns an insertion-ordered queue at
`+0x130c`, with the count at `+0x1310`. The player producers append to the
current local player's queue. The authoritative consumer uses the creature
whose queue it is visiting. On the wire, the connection supplies that creature's
identity. The source stack count stays with the producer.
(C `0x3b7f9`, `0x86100`, `0x69d95`; S `0x262c3`–`0x262e7`.)

| Offset | Width | Native meaning in these paths |
|---|---:|---|
| `0x000` | 280 | Item value, copied for kinds 1 and 6. Other proven player producers leave the default empty item. |
| `0x118` | 12 | Three signed 32-bit values for kinds 2, 3 and 5. They default to −1 each for kinds 1, 6, 7 and 8. |
| `0x124` | 4 | Initialized to 0. No use found in the traced dispatch. |
| `0x128` | 1 | Kind. Kind 4 and values outside 1 through 8 go to the common next-entry path. |
| `0x129` | 1 | No field semantics established. The initializer and field-wise copies skip this byte. |
| `0x12a` | 2 | Initialized to 1 and copied by the sender. No use found in the traced dispatch. |

Kind 2 identifies a creature's **home spawn** (zone x, zone y, spawn index).
Kinds 3 and 5 use the zone x, zone y and current index in that zone's static
object or dropped-item list, respectively. These records carry the selected
identity by value. A pickup subsequently re-resolves the index against the
then-current item list. Removal compacts the list, so a queued index can become
invalid or refer to the item that shifted into the slot. These are zone indices
and ordinals, with no persistent object token in the request.
(C `0x59530`, `0x594c0`, `0x969cb`, `0x974e2`, `0x9761a`, `0x2107b7`,
`0x19c800`; S `0x13630d`, `0x136506`, `0x13670a`.)

## Input and frame order

The window loop dispatches a pending Win32 message before polling input and
calling the frame. Right and middle mouse-down messages invoke the inventory
callback, which can use an item immediately. The focused input poll separately
samples the left mouse button and DirectInput scan codes for Q, R, E and T.
The gameplay frame tests down edges against its saved state. The input fields
are left mouse `+0x04`, Q `+0x17`, R `+0x0f`, E `+0x11`, T `+0x10` on the game
object. The frame writes the Q and drop edge histories at their respective
sites and the R, E and T histories later together. Distinct producer sites can
read the same R or E edge during one frame.
(C `0xc9057`–`0xc90a8`, `0xc8518`, `0xc8542`, `0xc91da`–`0xc92b5`,
`0xc9350`–`0xc935f`, `0xc9432`, `0xc85f0`, vtable at `0x2ffdc8`,
`0x9213b`, `0x966c9`, `0x9b593`–`0x9b5a6`.)

The frame at C `0x88ee0` runs all of its world pieces together at
`0x8cef4`–`0x8cf49`, after the seed and world-name checks at
`0x8ce9d`–`0x8cec9`. It passes each piece the same frame output accumulator.
Full pieces are 20 ms and the last is the remainder, up to 20 ms. For example,
45 ms gives 20, 20 and 5 ms. An exact 40 ms gives two 20 ms calls. A frame
that reaches this group with 0 ms still calls the last piece. The input
producer sites below all follow the entire group of world calls.
(C `0x787f0`, `0x51440`, `0x1a5324`, `0x20c510`.)

| Append source | Position in a gameplay frame when its conditions hold |
|---|---|
| Inventory bomb | Mouse-down callback before the world's pieces in the window-loop iteration that dispatches it. The callback enters the world critical section around the use. C `0x7d71d`–`0x7d736`. |
| Q bomb | First of the player frame append sites, after the world and the early grant handling. C `0x920ae`–`0x92136`. |
| Left-click drop | After Q. C `0x9662f`–`0x966c4`. |
| R talk | After drop, at the creature interaction site. C `0x96922`–`0x969fa`. |
| R static interaction | Later than talk, at the static object site. C `0x9735f`–`0x97511`. |
| E pickup | After static interaction. C `0x975bb`–`0x97649`. |
| T pet return | After pickup. C `0x976e7`–`0x9771e`. |
| R blueprint or quick-item wheel use | Later than T. Blueprint mode has its own branch. In the ordinary branch, the open quick-item wheel confirms with R or E and can append a bomb. C `0x9797d`–`0x979c7`, `0x97db6`–`0x97e76`. |

Earlier queued entries retain their place ahead of a later inventory callback
or frame append. There is no sorting by kind or by keyboard event time. A
callback bomb already queued when the first eligible world piece starts can
be consumed there in single player. A Q bomb and an E pickup from the same
frame are appended in that order and first become eligible in a later frame's
world piece. Even a long frame cannot use another piece to consume its own
late input appends. The table orders sites, and their gates still apply. The
target caches commonly exclude each other. For example, the item selection
path at C `0x99c60` also clears the creature and static selections.

## Player producers

### Kind 1 — bomb and consumables

The shared use routine accepts item type 1 when the local player's signed
hit-stun counter at `+0x12c` is below 1. A bomb is subtype 7. It appends kind 1
with a full copy of the item, then decreases the source stack by one. At zero
or below it sets the count, type and subtype to zero. Bomb enqueue applies
no action, healing, sound, or random draw. The inventory call is reachable
with a nonempty selected inventory stack, item usability check and appropriate
window state through a right or middle mouse-down. Q and the wheel call the
same routine for the selected inventory stack after the item-level check.
The Q site uses its rising edge in the gameplay input path. The wheel site uses
the R or E edge with the wheel open and closes it after the use call.
(C `0xa2780`–`0xa2854`, `0x7ce38`–`0x7d006`, `0x7d72b`, `0x3e420`,
`0x920ae`–`0x9213b`, `0x979cc`, `0x97db6`–`0x97e7b`.)

Other accepted type-1 items take the timed use path. A copy goes to the active
consumable field at creature `+0x1e8`, action becomes 80 for subtypes 1 and 2
or 81 for the other subtypes, and the action clock at `+0x6c` and the counter
at `+0x128` reset. The same stack decrement follows. Actions 80 and 81 evaluate
healing on 200 ms clock-bucket crossings while the previous clock is below the
item's duration. The world uses the active item and caps health after a
positive heal. The locally controlled creature follows this path online too.
The action has its own sound and hit outputs and no kind-1 append. A positive
healing tick reports a negative hit amount with attacker id −1 at C `0x218a39`.
Duration is 3000 ms for type 1 subtype 1 and 10000 ms for the other type-1 items
in this duration helper.
(C `0xa27f6`–`0xa2840`, `0xc6b80`, `0xc6e10`, `0x2188fd`–`0x218be3`, `0x21a1ea`.)

### Kind 2 — talk

On an R edge, the talk site resolves the current focused creature id from game
`+0x800a70`. The creature must exist, have controller byte 3 at creature
`+0x60` and pass the frame's interaction UI gate. It supplies the twelve bytes
of its spawn identity from `+0x1b0`. The producer can also update the dialogue
UI immediately. Inn time and actor-class checks belong to consumption. The
same site queues the talk record irrespective of whether dialogue can do
anything to the clock. The enqueue itself has no random draw.
(C `0x96922`–`0x96a4f`, talk prompt `0x301be8`.)

### Kind 3 — static interaction

The later R site requires the quick-item wheel flag clear. A dead local player
takes the revive branch. With positive health, the game runs its interaction
UI helper, checks the static selection at game `+0x800a84` and resolves the
entity. The generic queue branch copies this static identity into the record.
Kinds 45 and 77 and the station/object cases 65, 71, 72, 73, 74, 75 and 76
take dedicated UI or local paths at this site. In particular the direct R
branch for 45 sets local teleport-related flags. The existence of a kind-45
dispatch branch does not establish direct R production for that entity kind.
There is no draw to build or append the generic record.
(C `0x9735f`–`0x975b6`, `0x1a0910`.)

An NPC can append its own kind 3 through the timed interaction behavior. The
observed append uses the NPC's saved static identity at creature `+0x1478`
through `+0x1480`. At that site the entity must resolve and squared distance
in blocks must be strictly below 9. It clears the intent and movement fields
and sets the behavior timer to 20000 ms after enqueue. Choosing a target
earlier in the behavior can draw randomness. The append does not. This
behavior runs after the NPC's queue pass in its turn. Its own new entry is
eligible in the next world piece, which can be in the same frame.
(C `0xdb130`, `0xdbddc`–`0xdbeba`, `0x211af1`–`0x211b4d`;
S `0x1bc20`, `0x1c8e0`–`0x1c95b`.)

### Kind 5 — pickup

On an E edge, the producer requires a valid item target at game `+0x800a78`
and re-resolves it through the current loaded zone and item-list bounds. The
carry check counts matching held and inventory stacks of item types 1, 11 and
20 using native item equivalence. Types 1 and 11 are refused at a matching
count of 50 or more. Type 20 is refused with any positive matching count.
Refusal shows a capacity message and plays sound 49. Success queues the target
triple with an empty item. There is no producer-side inventory grant, world
removal, distance recheck against the dispatch's 4-block limit, or pitch draw.
The cached aim/selection has its own acquisition rules.
(C `0x8d773`, `0x975c1`–`0x976e2`, `0x19fb90`, `0x3e4a0`, `0x2f4a0`,
`0x99c54`–`0x99c92`.)

### Kind 6 — drop one held item

The left mouse down edge is tested in the frame. It requires a nonzero held
stack at creature `+0x11e8`, the open inventory-related widget at game
`+0x8008bc` and a zero result from the UI hit test. The record copies only the
held item value at `+0x11ec`. The producer appends first and then decreases the
held count by one, clearing the type and subtype at exhaustion. The drop's
position and randomness belong to consumption. Picking an item up with the
inventory cursor and dropping one into the world are separate operations.
(C `0xc9350`–`0xc935f`, `0x9662f`–`0x966cc`, `0x7fa10`, `0x250ae0`, `0x2f140`.)

### Kind 7 — blueprint placement

The R edge in the frame's placement branch appends kind 7 with a default item
and target. The branch requires the mode byte at game `+0x800704` to be set.
The image also has a preview branch gated by that byte. The constructor
initializes the byte to zero. A stock UI route that enables this mode remains
unverified. The consumer gets a signed selection byte from creature `+0x18d`
at consumption, uses it to look up the voxel object, and uses the creature's
then-current position and facing. The request carries neither the selected
number nor placement coordinates. A missing object lookup exits. Verified
outputs include block edits and the changed zone object list. The enqueue has
no draw. The full placement transform and indirect draws remain open.
(C `0x5a09b`, `0x9797d`–`0x979c7`, `0xb3338`–`0xb335e`,
`0x2112f7`–`0x2119c2`; S `0x1370b7`.)

### Kind 8 — pet return

The T down edge appends kind 8 with a default item and target. At consumption,
the requester supplies the pet's id from creature `+0x11c8`. A present pet
clears threats and moves to the requester if its squared 3-D distance exceeds
40000 blocks². The requester's own per-target hit counters and target id are
cleared whether or not a pet resolves. A dead pet can move without reviving.
There is no draw in the traced producer or handler.
(C `0x976e7`–`0x9771e`, `0x210699`–`0x210741`; S `0x136459`.)

## Dispatch, outputs and draw order

The world visits each creature's requests at the start of its stage-12 turn,
before health clamping, the dead-creature skip, AI and physics. It dispatches
all pending entries in insertion order, even if a dispatch produces no effect.
On an authoritative step it clears the list after that creature's pass. This
applies both to the offline client and the dedicated server. In online client
mode the shared step can still walk that list, and leaves the list intact for
the sender. The online flag is world `+0xb4`, set by the connection path at
C `0x6fca7`. It is clear on the dedicated server's authoritative step.
(C `0x2104ec`–`0x210546`, `0x211a44`–`0x211adb`, table `0x222c20`;
S `0x1362ac`–`0x136306`, `0x137804`–`0x13789b`, table `0x1489e0`.)

| Kind | C handler RVA | S handler RVA | Verified dispatch behavior |
|---|---:|---:|---|
| 1 | `0x2110a4` | `0x136e64` | Type 1 subtype 7 creates a type-144, controller-6 bomb at the requester's current position with the item's signed level. Its id is one below the minimum of zero and the occupied ids. The type-144 appearance branch has no draw. The generic heal arm at C `0x211212` applies the item's heal value and emits sound 44 and a negative hit amount with attacker id 0. |
| 2 | `0x21054d` | `0x13630d` | Searches current creature-table order for matching home spawn identity. A class-132 match at or after 18:00 or at or before 06:00 increases the day and sets 07:00, emits sound 29 at the requester with volume 0.75 and runs the spawn/mission reset. A matching appearance flag `0x80` otherwise stops the search. |
| 3 | `0x21094a` | `0x13670a` | First clears action 79 and the associated field at creature `+0x1190`. Looks up the static entity by current zone and index. Local-only entity kind 45 and the early action handling are covered below. Seats require a free entity and action 0, 83 or 84. Kind 10 state 2 drops contents. Kinds 1, 2, 3, 9 and 10 in state 0 can activate and report static changes. |
| 4 | `0x211a44` | `0x137804` | Advances past the entry. No producer established. |
| 5 | `0x210746` | `0x136506` | Requires the current zone and index, item ownership eligibility and squared distance at most 16 blocks². Adds the zone to the changed-item set and emits one addressed section-9 record. If the requester is the world's local creature, it also immediately adds the ordinary item to that creature's inventory. It then draws pitch for sound 45 and removes the item. |
| 6 | `0x2119cc` | `0x13778c` | Invokes the common world-drop routine at the requester's current position with scale 1.0, then emits sound 57. Sound submission follows the drop attempt even if the target zone fails to resolve. |
| 7 | `0x2112f7` | `0x1370b7` | Uses the current selection byte and places the object through the voxel placement path. |
| 8 | `0x210699` | `0x136459` | Resolves and resets the pet and clears the requester's targeting state as described above. |

Kinds 1, 2, 5, 6, 7 and 8 skip their effects when the world's online flag is
set. Kind 3's action-79 reset is before target validation and that mode check.
The kind-45 branch can act for the world's local creature even in the online
client step, given a valid destination. For eligible free-seat cases online,
the branch zeroes the creature vector at `+0x40` without locally assigning the
seat. The authoritative branch binds the seat. Chest release and ordinary
static activation require the authoritative mode.
(C `0x21094a`–`0x210b09`, `0x210b2b`–`0x210b78`, `0x210da4`–`0x210dea`.)

Successful pickup draws once at C `0x2108ba` or S `0x13667a`. Sound 45 pitch
is 1 plus the single-precision product of the draw and 0.1 divided by 32767.
The type-25 ownership check compares the 32-bit item field at `+0x04` with the
requester's artifact counter at creature `+0x1164`, folded to 100 through 199
if above 100. Other types pass that ownership predicate. Only the producer
checks carry capacity. Type 25 bypasses the ordinary direct inventory add.
A failed pickup takes no pickup-pitch draw.
(C `0x3e550`, `0x2107c8`–`0x21090c`; S `0x136506`–`0x1366cc`.)

The world-drop helper draws the `+0x13c` timer first, using 500 plus the
integer remainder of a draw divided by 300. A loaded zone then permits a
second draw for the stored angle, scaled by 360 and divided by 32767 in
single precision. This is one draw even on a no-zone drop attempt and two
when that zone lookup succeeds. The chest-release path consumes per-unit
preparatory draws for two horizontal offsets and an orientation argument
before invoking that same helper. It draws and drops per unit in loot list
order, decrementing the chest's stack afterward. These draws occur at
kind-3 consumption, after a chest in state 2 resolves. The positive drop timer
is decreased in the earlier world-item pass of subsequent pieces. Crossing
zero clamps it to zero. The authoritative update then draws one pitch with
the same transform as sound 45 and emits sound 58 for item type 12 or 59 for
other types, at the item's position.
(C `0x19aeb0`, `0x19af2e`, `0x19afb9`, `0x20d1ed`–`0x20d29e`, `0x210de3`–`0x210f5f`;
S `0xd2810`, `0x1377c3`.)

## Result delivery during the frame

Single-player pickup has two distinct effects in the piece. The authoritative
handler immediately credits the local ordinary item and also emits the
recipient id plus item in section 9. It records sound 45 and marks the zone,
whose item-list update is emitted in section 7 at world wrap-up. The item can
therefore be available to a later input site in the same frame. Dropping and
chest output create world items through the common helper in the consuming
piece. Normal frame sound playback is after the world pieces at
C `0x955b1`–`0x95685`. There is no per-piece replay of the whole output accumulator.
(C `0x210833`–`0x210940`, `0x19b140`–`0x19b163`, `0x221180`–`0x2211ee`;
S `0x1365e5`–`0x13670a`, `0x146efc`–`0x147222`.)

Online, Packet 4 is decoded by the receive thread. The frame applies a received
item-list replacement to the loaded zone before looking up the cached item
selection. The receive thread appends grants to the protected pending list at
game `+0x800670`. In the frame, pending grants resolve the recipient creature
id. Only a match to the local creature adds an ordinary item to inventory
and updates the inventory UI. The frame
moves those previously pending grants into the shared output/result pass.
Fresh local world outputs are already in the output accumulator, so they
skip that prior pending-list insertion. This avoids a second inventory add
for offline pickups. The shared result pass handles pickup messages, item
type 13's local experience effect of ten times the item's signed level, and
the type-25 artifact progression path separately from normal item insertion.
Type 13's experience change checks the local recipient id. The type-25 branch
increments the local artifact counter after a recipient resolves and has no
additional local-id equality test at this site.
These grant passes follow all the world pieces and precede the Q input site.
(C `0x6cc0e`, `0x6ce8c`–`0x6d088`, `0x8d640`–`0x8d79b`, `0x8f719`–`0x8f84f`,
`0x8f858`–`0x8facc`, `0x80fb0`.)

The frame also has a separate local pending-item path before the world pieces.
If its list at game `+0x1000e68` is nonempty and its accumulated clock is
strictly above 100 ms, it resets the clock, adds one front item to the local
inventory, plays sound 45 at pitch 1, removes the entry and refreshes the UI.
This path is outside Packet 6 dispatch and its pitch draw. The sources of
every entry to that auxiliary list remain outside this contract.
(C `0x8cd99`–`0x8ce29`, `0x20aa50`, `0x86080`.)

## Online pacing and ownership

The sender's world critical section covers snapshotting the local creature and
the outgoing queues. On a nonempty request queue, it copies and removes one
front entry and decrements the count before releasing the lock and sending
the request. The frame and the inventory use callback take this same world lock around
their producers. The send order is packet 0 for the player, optional front
chat packet 10, optional packet 6, then all queued 7, 8 and 9 in that order.
The 11 and 12 request sends have a separate strictly-above-1000-ms gate. The
loop has a 20 ms wait target and no packet-6 acknowledgment or retry queue.
A four-byte packet id precedes the 300-byte record on the wire.
(C `0x69c10`, `0x69d95`–`0x69e35`, `0x6a095`–`0x6a0e7`,
`0x6a259`–`0x6a380`, `0x8902f`, `0x9d2ea`, `0x201cb0`, `0x201e90`.)

The server receive case appends to the sender's creature queue under the
world lock. A server step can consume several requests already appended to
that creature, with no one-per-step dispatch cap. The server groups results
into normal world updates for clients. The connection's own send cadence
does not establish the step on which a particular request is consumed.
There is no guaranteed cross-player arrival order through the stage-12
creature traversal.
(S `0x262c3`–`0x262e7`, `0x14a0ae`, `0x1362ac`–`0x137851`, `0x23dd0`.)

## Evidence limits

This contract establishes native append sites, their proven gates, the local
phase boundary, and the source of the ordinary grant and sound results.
Blueprint-mode activation, complete geometry selection and placement rules,
all static activation subcases, and runtime timing across network threads
remain open. Treat the timed consumable path, the explicit kind-1 heal arm,
and the separate pre-piece pending-item drain as distinct native paths. A
dispatch case by itself proves only consumption.
