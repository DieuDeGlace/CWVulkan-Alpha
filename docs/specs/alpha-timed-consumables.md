# Alpha timed consumables

Status **FINAL / STABLE / HELD**

This contract covers stock inventory and quick-item use, the shared world-step
effects of actions 80 and 81, and their client presentation. All routine addresses
are RVAs in the supported Alpha images. Both images have base 0x400000.

| Image | SHA-256 | World step RVA |
|---|---|---:|
| C (Cube.exe) | 84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf | 0x20c510 |
| S (Server.exe) | 1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1 | 0x1322d0 |

Creature offsets use the whole in-memory creature object. Item offsets begin at
the 280-byte item payload. A stack has a signed 32-bit count followed by that
payload. Packet section numbers refer to the server-to-client world update
in packet 4. The event field offsets below begin at each event record.

## 1. Player admission

A right or middle inventory mouse-down can reach the use branch with the
appropriate window state, a selected stack and a nonzero count. The inventory
checks general item usability, which compares the transformed item and player
levels before the item/class check. A usability rejection requests sound
0x32 at the listener. The type-1 arm calls the use routine while holding the
world critical section. (C 0x07ce38–0x07d006, 0x07d717–0x07d73b,
0x03e420, 0x0c76a0, 0x0c6f20, 0x201cb0, 0x201e90.)

The gameplay Q edge uses the retained quick-item selection. With the wheel
open, the later confirmation site checks an R edge first, or an E edge when
R has no edge. Both resolve the stack and compare its live signed 16-bit item
level at item +0x10 to the creature's signed 32-bit level at +0x190. Failure
requests sound 0x32 at the listener. Passing wheel confirmation calls the use
routine and then closes the wheel, including when the use routine rejects.
Q and wheel confirmation follow that frame's world pieces. A mouse event
can take effect before the pieces. The candidate and input gates are given in
[the quick-item wheel contract](alpha-packet6-quick-wheel.md).
(C 0x0920ae–0x092145, 0x097db6–0x097e82, 0x08528a.)

The shared use routine requires item type 1 and signed hit-stun at creature
+0x12c at most zero. A timed use copies the selected payload to creature
+0x1e8, sets action byte +0x68 to 80 for subtypes 1 and 2 or to 81 for the
other timed subtypes, and zeros signed 32-bit elapsed time +0x6c and field
+0x128. It then decrements the stack count. At a resulting count at most
zero it clears the count, type and subtype. The use helper permits a new
accepted use to replace an active one and restart its clock. The timed
admission itself makes zero game random draws and produces no immediate
heal, sound, particle or packet-6 request.
(C 0x0a2780–0x0a2854, copy at 0x02c5e0.)

Subtype 7 takes the bomb path. It queues the copied item in packet-6 kind 1
before the common stack decrement. The distinct native kind-1 handler also
has an immediate non-bomb heal arm with source ID 0 and sound 44. See
[the packet-6 client queue contract](alpha-packet6-client-queue.md)
for that request's transport and dispatch.
(C 0x0a27bb–0x0a27f4 and 0x2110a4, immediate heal C 0x211212 and S 0x136fd2.)

## 2. World-step eligibility and cadence

The world step first caps a creature's existing health at its maximum and
skips the later creature processing when health is at most zero. Before the
timed effects it subtracts the current piece's milliseconds from the signed
hit-stun field. With positive stun remaining it writes zero to the action
and its elapsed field. The normal clock for an active timed action advances
by the piece's milliseconds before evaluating effects. Controller kind 6
and a negative elapsed clock with nonzero action bypass that clock increment.
The ordinary action setup at zero elapsed makes no game random draw for
80 or 81. The client splits world time into pieces of at most 20 ms.
(C 0x211a96–0x211adb, 0x2184c2–0x218541, 0x03e6a0, 0x08cef4–0x08cf49.
S 0x137856–0x13789b, 0x13e282–0x13e301.)

After those stages, action 80 or 81 can produce a timed effect when the
creature is the world's local creature at world +0xb8, or when the world
byte +0xb4 is zero and creature controller kind +0x60 is nonzero.
(C 0x2188fd–0x21892d, 0x06fca7.
S 0x13e6bd–0x13e6ed, 0x149cb7–0x149cbd, 0x0c867c–0x0c8682.)

| World | Eligible actors in ordinary play |
|---|---|
| Offline client | The local player and nonzero-controller creatures. |
| Connected client | The local player. Other actors can receive and display their replicated actions. |
| Dedicated server | Nonzero-controller creatures. The local-creature pointer is null and a connected player starts with controller kind 0. |

The active item's duration is 3,000 ms for type 1 subtype 1, 10,000 ms for
the remaining type-1 subtypes, and zero for other types. At the effect gate
let *t* be the signed action elapsed time and *p* be *t* minus the current
piece's milliseconds in signed 32-bit arithmetic. Effects require *p*
strictly below the duration and different signed quotients for *p* and *t*
when each is divided by 200. Signed division truncates toward zero. A piece
that crosses several boundaries still admits one tick. A tick at the nominal
endpoint is eligible when its piece began below the duration. At each admitted
tick the fraction is binary32 200 divided by the duration, followed by a
binary32 multiplication by 0.5 when controller kind is nonzero. Within that
tick, different signed quotients upon division by 400 request the recurring
sound after the health and secondary-accumulator work.

| Image | Duration helper | 200 ms effect test | 400 ms sound test |
|---|---:|---:|---:|
| C | 0x0c6b80 | 0x21892d–0x21897b | 0x218b61–0x218b90 |
| S | 0x013aa0 | 0x13e6ed–0x13e73b | 0x13e921–0x13e950 |

Both images use the cadence recorded for C in
[the item contract §9](alpha-item-names.md#9-healing-of-actions-8081-open-point-i-1).
The earlier 100 ms effect and 200 ms sound reading is superseded for both
checked images. [The pets and bombs contract §6](alpha-pets-bombs.md#6-packet-6-kind-1-for-other-items).

An uninterrupted use from elapsed zero, with pieces crossing at most one
boundary each, produces 15 ticks for 3,000 ms or 50 for 10,000 ms.
The uncapped aggregate health gain is approximately the item's heal value
for controller kind 0 and half that value for other controllers, subject to
binary32 rounding.

## 3. Ordered tick outputs

**Health.** The type-1 subtype base is 200 for 1, 2, 4, 5 and 6, 100 for 8
and 9, and zero otherwise. The level input is the signed item level and the
rarity input is the unsigned byte at item +0x0c. To obtain the item's
scale factor, subtract one from the level, multiply by binary32 0.05 and
add one. Subtract the reciprocal of this result from one and multiply by
three. Raise two to that exponent and separately raise two to the exponent
0.25 times the rarity. The exponent arithmetic uses binary32, the power
calls use binary64, and each power result is rounded to binary32 before
multiplication. Multiply the resulting scale factor by the subtype base,
then by the admitted tick fraction, preserving binary32 operations. Call
the resulting amount *H*.
(C 0x0c6e10, 0x045f60, 0x218981–0x2189eb.
S 0x013be0, 0x010f00, 0x13e741–0x13e7ab.)

For positive *H*, first append a 72-byte ordinary hit to the section-2
output, then add *H* to binary32 health at creature +0x16c and cap at current
maximum health. The hit reports the full pre-cap amount even at full health.
A zero gain skips both the hit and the health change. The hit has the
following fields. (C 0x2189f1–0x218aa4, 0x059a30, 0x086290, 0x044db0.
S 0x13e7b1–0x13e864, 0x022a90, 0x028400, 0x00fda0.)

| Offset | Value |
|---|---|
| +0x00 | Signed 64-bit source ID −1, all bits set. |
| +0x08 | Signed 64-bit target ID copied from creature +0x08 of the consuming creature. |
| +0x10 | Binary32 amount −*H*. |
| +0x14, +0x18, +0x1c | Critical byte, stun and knockout values zero. |
| +0x20 | Three signed 64-bit coordinates copied from creature +0x10, in fixed units of 1/65536 block. |
| +0x38–+0x46 | Zero knockback and flags, including the kind-0 byte at +0x45. |

**Secondary accumulator.** This part runs on every admitted tick, including
when *H* is zero. For type 1 subtype 4 or 6, divide the item scale factor
above by the binary32 result of raising two to three and multiply by 1.5.
Other types and subtypes contribute zero from this item helper. Divide the
item helper's result by two raised to binary32 0.25 times the consuming
creature's signed level at +0x190, with the power result rounded to binary32.
Multiply by the tick fraction to obtain the increment. Divide, multiply and
add in binary32. Store the sum of the signed binary32 value at creature
+0x13c and the increment. If the sum remains negative, retain it. When the
sum is at least zero, subtract the increment from that rounded sum. A
negative result of this subtraction appends sound 47, then the field is set
to positive zero. The nonnegative subtraction result also clears the field
to positive zero. (C 0x0c7a90, 0x045ff0, 0x011d10, 0x218aa4–0x218b57.
S 0x014200, 0x010f90, 0x0055a0, 0x13e864–0x13e917.)

**Sound.** The recurring sound is 44. If both sounds are admitted together,
47 precedes 44 in the sound list. They are 24-byte section-4 records with
sound ID at +0x0c and pitch and volume both set to binary32 1 at +0x10 and
+0x14. The position is the creature's signed fixed-point coordinate triple,
each converted to binary32 and multiplied by binary32 1/65536, at +0x00.
The client drains these entries after its floating-hit and independent
particle passes. Its sound ID map selects drink2.wav for 44 and
upgrade2.wav for 47. The timed world-step arms append no section-3 particles.
Timed admission, the tick computations and appends, and the traced sound and
heal-display paths make zero game random draws.
(C 0x218b1e–0x218bd6, 0x065fb0, 0x02c4a0, 0x0864c0, 0x0955b1–0x095630,
0x084350, 0x085168, 0x085216, 0x085e90. S 0x13e8de–0x13e996, 0x0c8530, 0x002550,
0x028590.)

## 4. Completion and presentation

Later in the world pass, action 81 becomes 83 on the first elapsed value
strictly greater than the item's duration. This transition leaves the clock
and stored item in place. At or below the duration the branch keeps 81 and
zeros that pass's movement scalar. The completion check is outside the
timed effect ownership gate. It skips action 80. The normal player action
expiry check also excludes both 80 and 81. An action-80 effect window ends
by the previous-clock cutoff in §2. A general cancellation later in the
world pass clears these actions when +0x128 is nonzero. An existing action
83 or 84 clears when the movement vector at +0x40 has positive length.
No stack restoration occurs in these timed branches.
(C 0x21a1ea–0x21a266, 0x09836b–0x098375, 0x2188cf–0x2188f9.
S 0x13ffaa–0x140026, 0x13e68f–0x13e6b9.)

The client creature renderer resolves the active item model for actions
80 or 81 while elapsed is strictly below the duration. If the model exists,
it draws at the animated attachment. At exactly the duration the item is
already hidden even though an action-81 creature may still be in 81 and
the endpoint tick may emit its outputs.
(C 0x020deb–0x020f4b, 0x0ec400.)

The client hit-display pass admits a timed heal number when the resolved
source or target is the local creature and the kind-0 magnitude is at least
binary32 0.1. For a local recipient, the text uses RGBA (0.4, 1, 0.5, 1),
size 20, lifetime 1,000 ms, a 100-unit screen displacement parameter and
centering flag 1. The position starts at the hit's position, adds two blocks
to Z and adds the fixed-point conversion of binary32 one minus half the
recipient's size component at creature +0x88. The local-heal branch then
subtracts one block from Z. The displayed value is the positive magnitude.
Below one, multiply by ten, truncate, multiply by binary32 0.1 and use the
numeric stream formatter. At or above one, truncate to an integer.
(C 0x094e0e–0x095241, 0x0664b0, 0x02c580, 0x012080, 0x02ca00.)

## 5. Connected delivery

The connected client's own timed effect is applied directly in its world
step and feeds the local display and sound passes. The outgoing hit filter
queues packet-7 hits only when their source equals the local creature ID.
The timed heal's source sentinel −1 leaves the normal player hit out of
that queue. Local timed sound uses the client event list. The usual packet-0
creature fields carry the action, clock, secondary accumulator, health and
active item. The server binds that state to the connection's creature and
the client ignores received updates for its own ID.
(C 0x09c02f–0x09c0b4, 0x069f81, 0x0ccfa0, 0x06bcc5.
S 0x0260df–0x02625f, 0x016210.)

The dedicated authority's eligible nonzero-controller creatures emit timed
hits and sounds in packet-4 sections 2 and 4. Section 2 is copied to peers
without distance filtering. Section 4 is limited to sound positions at
squared distance strictly below 40,000 square blocks from the connection's
creature. An incoming timed hit can reach health application on the receiving
client when that client controls the target. See the packet contract for
actor and event transport. [Alpha network packets](alpha-net-packets.md).
(S 0x0272d0, 0x024d30–0x024fa6.)

## 6. Authority controller admission and random order

A controller method in the checked vtables is a second way to start
action 80. The world calls its installed controller method for living
nonzero-controller creatures with world +0xb4 zero, nonpositive hit-stun
and accumulated controller time at least 32 ms. The method clears action
to zero when elapsed exceeds 10,000 ms and the action is outside 79 and
82–84. It then returns early for nonzero creature +0x128 or current action
80 with elapsed below 3,000 ms. Below one quarter of maximum health, when
elapsed exceeds the current action's duration, it takes the first nonzero
type-1 subtype-1 stack in bag and slot order. It sets 80 and elapsed zero,
copies the item,
then decrements the count and clears count, type and subtype on exhaustion.
It makes two consecutive native game draws after decrementing. The first
draw supplies movement X, the second movement Y. Convert each draw to
binary32, multiply by four, divide by 32767 and subtract two in binary32.
Movement Z is zero. The movement values are at creature +0x40, +0x44 and
+0x48. The shared tick path adds no later draw for the potion.
(C 0x211ae1–0x211b4d, vtable 0x2fd310, 0x02cb20–0x02cd20, 0x03d1a0,
draws at 0x02ccc1 and 0x02cce8.
S 0x1378a1–0x13790d, vtable 0x1587e0, 0x002f40–0x003140, 0x0084b0,
draws at 0x0030e1 and 0x003108.)

In the same controller method, a branch with no resolved target creature,
health below half maximum, and controller kind 3 starts 80 at elapsed zero.
It writes type 1, subtype 4 and the low 16 bits of the creature's level to
the active item without copying a stack. Other active item bytes retain
their prior values. The start branch zeros the movement triple and executes
zero game draws. (C 0x02ce36–0x02cee7. S 0x003256–0x003307.)

The gameplay label and other writers of the secondary accumulator, the full
controller selection and interruption policy, the exact held-item joint and
pose transforms, and live per-peer presentation latency remain open.
