# Alpha connected client hit receipt, presentation and send

**FINAL / STABLE / HELD**

Evidence is the original 32-bit Alpha `Cube.exe`, 3,885,568 bytes,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs for image base `0x00400000`. Scope is the connected
client's ordinary socket path with a resident local creature and
valid records. The local pointer is game `+0x8006D0`. ID resolution
looks in the live creature map of the world at game `+0x2E4`.
Numeric stages stated as binary32 round separately.

[alpha-connected-melee-candidates.md](alpha-connected-melee-candidates.md)
covers the action-1 hit producers. The deferred action-zero type-4
producer is in
[alpha-connected-combo-cleanup.md](alpha-connected-combo-cleanup.md).
This contract follows ordinary type 0, guard type 1, protection
type 3, deferred type 4 and ward type 5. A self heal is a type-0
record with equal endpoint IDs and a negative amount.

## 1. Records and collection boundaries

A hit payload occupies 72 bytes. The fields relevant here are as follows.

| Record offset | Field |
| --- | --- |
| `+0x00` | 64-bit source creature ID |
| `+0x08` | 64-bit target creature ID |
| `+0x10` | Binary32 amount |
| `+0x14` | Critical byte |
| `+0x18` | Signed 32-bit stun duration |
| `+0x1C` | Additional 32-bit field copied by the record helper |
| `+0x20` | Signed 64-bit X, Y and Z in 1/65,536-block units |
| `+0x38` | Three binary32 impulse components |
| `+0x44` | Resource indicator byte |
| `+0x45` | Hit type byte |
| `+0x46` | Auxiliary effect byte |

The copy helper at `0x0599A0` preserves these members. The wire paths
transfer 72 bytes per record in native little-endian layout. Padding
contents are outside this contract. The list helper `0x086290` appends
a copy at the tail. The ordinary and self-heal producers that link
directly also append at the tail (`0x197A11–0x197A5D`, `0x198677–0x1986B2`).

The game frame constructs a fresh output collector at `0x08CE2E–0x08CE35`.
Its hit list starts at collector `+0x00`, sound at `+0x08` and
outgoing buff events at `+0x58`. A frame that runs world simulation
passes this same collector through its 20 ms subdivisions and the
last residual step (`0x08CF20–0x08CF49`). Records already in the
collector retain append order through those steps.

After those calls the frame walks received pending hits once for
local target state. A separate walk then adds eligible received
records at the collector hit tail and the pending list is cleared.
This places the received tail after hits produced during the
world step. All later hit presentation and the socket queue copy
walk the collector from its head to its tail with their own filters.
Evidence is `0x08FECB–0x09034A`,
`0x090403–0x0904B0`, `0x0905BD–0x0905C4`, `0x094971–0x0952A5`
and `0x09C02F–0x09C0F4`.

## 2. Received hit intake and the two frame filters

The connected receive dispatcher selects the server world-update
route for 32-bit incoming message ID 4. It reads a 32-bit byte
length, reads that many bytes, and transforms the block through
`0x0494B0` before reading its sections. The hit section is the
second decoded collection, after the section read by `0x058310`.
Its decoder `0x0583F0` reads a signed 32-bit count and, for a
positive count, takes each 72-byte record into the temporary list
in wire order. The destination copy at `0x06CF26` appends to the
pending hit list at game `+0x800630` in that same order. This append
is within the receive thread's world critical section at world
`+0x8000C0`. A subsequent received batch appends after earlier
pending records. Evidence is the dispatcher at `0x06B8A7–0x06B8B7`
with tables `0x06D180` and `0x06D1A4`, receive and decode sites
`0x06CC0E–0x06CCFD`, range append `0x06CF0C–0x06CF26`,
`0x057B80`, `0x0540C0`, and lock sites `0x06CEE8`, `0x06D088`,
`0x201CB0` and `0x201E90`.

The state pass resolves record target ID `+0x08` through `0x02F000`
and compares the resulting pointer with the local creature pointer.
A match takes the local-state route in section 3. A missing or nonlocal
target advances to the next record. The state test admits a match
regardless of source ID (`0x08FF10–0x08FF37`).

The second pass compares the two 32-bit halves of the record's
source ID directly against the local creature ID at `+0x08`.
It copies the record into the frame collector only if the source
ID differs. This test does not need either endpoint to resolve.
The earlier state pass has already examined their targets. The
pending list is then cleared as a whole. The copy preserves the
payload fields
(`0x090440–0x090475`, `0x0905BD–0x0905C4`, `0x06F870`).

## 3. State updates for a received local target

Each pending record completes its local state work before the
iterator advances. The connected frame handles the following
branches here. It does not call the authority-side hit application
at `0x195A60` for these received records.

### 3.1 Guard, type 1

With positive amount, first copy the three impulse components to
local velocity `+0x34` and set binary32 local `+0x1184` to one.
The following guard branch always copies the same impulse to
velocity. The second copy is an assignment to the same fields, so
positive records perform two assignments of the impulse value
(`0x08FF3D–0x08FF92`, `0x011E30–0x011E49`).

Divide the record amount by the current local guard-stat result
from `0x03E190`. Add old local `+0x174` to mana `+0x170` and
pending resource `+0x144`, in binary32. Cap mana at one and pending
resource at the stored mana. Subtract the quotient from `+0x174`
and floor it at minus one. This branch ends before the health and
stun arithmetic. Its entry depends on the resolved local target
and type byte 1. Current guard action, health and positive guard
resource are not tested here (`0x08FF97–0x09007C`).

### 3.2 Ward, type 5

Walk local resident buffs at `+0x1178` in list order. For each
type-6 record subtract the full incoming amount from its binary32
strength at buff `+0x04`. A result at or below zero writes both
strength and signed duration `+0x08` to zero. The record remains
resident at this site. After each matching write, append a buff
event to this frame's output collector. The event uses the local
ID at both endpoints and carries that updated resident record.
This includes its own stored resident source ID. Every matching
buff receives the amount, and an empty or nonmatching list ends
the route with no ward event. Health and stun processing is
outside this branch (`0x090081–0x0901CB`).

### 3.3 Other scoped types, including self amount

Types 0, 3 and 4 take the general received-local branch. In
particular the type-3 test bypasses the preliminary positive-guard
impulse copy and proceeds to this branch. It requires local
binary32 health `+0x16C` strictly positive.

1. If amount is strictly positive, reduce local meter `+0x1190`
   by the separately rounded product of amount and 0.25 divided
   by the following level scale. Subtract one from the signed
   level at `+0x190`, convert to binary32, and multiply by 0.25.
   Use that exponent with base two in `0x011D10`, which promotes
   the rounded operands to binary64 and rounds back to binary32.
   Floor the resulting meter at zero.
2. If record stun `+0x18` is strictly positive, store the greater
   of that duration and signed local `+0x12C` in `+0x12C`.
3. Subtract the signed binary32 record amount from health. Floor
   health at zero, then cap it at the current maximum-health result
   from `0x044DB0`. A negative amount can heal a living local
   target. This branch makes no impulse copy.

A received type-4 record of the native zero-amount, zero-stun
shape does not invoke the dodge-benefit helper at this site. Its
ordinary health operation subtracts zero and checks the caps.
A displayed protection label and a received type-3 health update
can both occur for the same nonzero record. Evidence is
`0x08FF3D–0x08FF7F`, `0x090081–0x090083` and `0x0901D0–0x09030A`.

## 4. Shared frame presentation

The hit presentation walk begins at `0x094971`. Every record first
needs its target ID to resolve to a nonnull creature. It then
performs the following bookkeeping and effect gates before deciding
whether to enqueue text.

- When source also resolves and amount is strictly positive, add
  the full binary32 amount to source `+0x13C8` and reset the source
  signed age field `+0x13D0` to zero. This uses every hit type
  that passes those tests. The earlier per-creature frame pass
  increments `+0x13D0` by elapsed milliseconds and clears fields
  `+0x13CC` and `+0x13C8` if that age strictly exceeds 10,000, then
  adds elapsed to `+0x13CC`
  (`0x0948F0–0x094937`, `0x0949B0–0x094A31`).
- If source ID equals the local ID, the resolved target's
  controller byte `+0x60` is one, and game `+0x8006D8` is null,
  store that target pointer at `+0x8006D8`. Type and amount add no
  conditions here (`0x094A31–0x094A79`).

Resource byte `+0x44` nonzero and a resolved source admit the
weapon-light pass. It collects the source's current weapons
through `0x03DF60`, using the live state at presentation. For each
weapon `0x0C6CC0` averages the four binary32 color components
returned by `0x0C7250` for its enhancement entries of type at
least 128, evaluated at factor one. It averages by adding in entry
order and multiplying by the reciprocal of the count. The
weapon emits when the fourth component is strictly positive.
Append a light at the record's fixed position, with the first
three color components, radius 20, age zero and lifetime 400 ms,
to game `+0x800748`. Independently, auxiliary byte `+0x46`
nonzero with a resolved source appends a light from the same
record position, with color (0.3, 0.3, 0.5), radius 16, age zero
and lifetime 400 ms. This second path recognizes received
records with that byte. The action-1 producers in scope use
auxiliary zero. The light age advances by frame elapsed time and
entries are removed when strictly past lifetime. The lighting
walk takes the stored radius and multiplies RGB by binary32
one minus age divided by lifetime, floored at zero. Evidence is
`0x094C00–0x094E0E`, `0x0C6CC0–0x0C6DFF`, `0x096402–0x0964A7`
and `0x0AF4D0–0x0AF583`.

### 4.1 Floating labels and numbers

Text additionally requires the resolved source pointer or resolved
target pointer to equal the local creature pointer. An unresolved
source is compatible with text if target is local. Types 4 and 6
skip floating text. Type 0 skips it when absolute amount is
strictly below binary32 0.1. For admitted records the text is as
follows (`0x094E0E–0x094E85`, `0x095144–0x095206`).

| Type | Floating text |
| --- | --- |
| 0 | Absolute amount in the native wide stream. Below one, truncate ten times the magnitude toward zero and multiply the converted result by binary32 0.1 before insertion. At or above one, insert the magnitude truncated to a signed integer. |
| 1 | `BLOCK` |
| 3 | `MISS` |
| 4 | No floating text |
| 5 | `ABSORB` |

The strings are in the original image at `0x301BB8`, `0x301BC0`
and `0x301BC8`. Negative amounts use RGBA (0.4, 1, 0.5, 1).
Nonnegative amounts use (1, 0.3, 0.3, 1) for a local target and
(1, 1, 0.25, 1) for a nonlocal target with resource byte nonzero.
Other nonlocal targets use (1, 1, 1, 1). A negative-type-0 heal
therefore displays the positive magnitude in its heal color.
The label starts at the record's fixed position, shifted upward
by two blocks, then by the fixed conversion of binary32 one
minus half of the target's live size Z at actor `+0x88`. A
local target subtracts one additional block from the anchor.
A local target uses initial motion scalar 100. A nonlocal target
uses minus 100, except a nonnegative critical amount uses minus
200. Critical byte nonzero sets size 60, otherwise size 20.
The label is appended to game `+0x800738`, with age zero and a
1,000 ms lifetime (`0x094E85–0x095134`, `0x095206–0x095241`,
`0x0664B0`).

The later label queue walk projects the stored anchor with the
current camera. Its draw route requires positive projected depth.
It advances the age even when the depth test fails and removes
labels whose incremented age strictly exceeds their lifetime
(`0x09586B–0x09594A`, `0x095E1F–0x095ED9`). The hit text and
light gates are separate from the frame sound-list walk.
That later walk takes each sound's own position, ID, pitch and
volume to `0x084350` (`0x0955B1–0x09568B`). The hit presentation
loop adds no direct sound or random draw in the scoped routes.

## 5. Socket queue and sender

The late frame send-collection route requires game socket dword
`+0x8006CC` nonzero. In frame list order, copy each hit whose
full source ID equals the local creature ID into the tail of game
hit queue `+0x8006EC`. Neither target resolution nor type, amount
or ownership is retested here. Presentation did not remove the
hit records from that frame list. A local-source record can
therefore enter this queue even when it could not be presented.
While the local ID stays the same, the merge's earlier source filter
keeps received tail entries ineligible for this copy
(`0x09C02F–0x09C0F4`, `0x095266–0x09529F`).

The next frame collection pass copies local-source buff events
from collector `+0x58` into game `+0x8006F4` with the same exact
64-bit source ID comparison. Thus a local defender's self-addressed
buff from the producer's dodge benefit, or a local ward update
copied from a received hit, can enter the buff queue independently
of the corresponding hit. Received buff input has a separate
frame route at `0x09034A–0x090401`
(`0x09C0F4–0x09C1A5`).

The sender at `0x069C10` takes the world critical section, copies
the pending hit, buff and third output lists, clears the game
lists and copies current local identity and position before
releasing the section. The hit snapshot copies records in list
order. It is removed from the game queue before the socket
writes for that snapshot. The hit send pass walks the copied
list from head to tail, after the normal entity-state send and
the optional preceding message-6 path. For each entry it sends
a four-byte message ID 7 and then the 72-byte hit payload
directly. Each hit has its own header. There is no hit-count
prefix on this route. The buff snapshot follows, using message
ID 8 and a 40-byte payload per entry. These are the connected
socket sender's outgoing messages. Evidence is
`0x069CDB–0x069D2B`, `0x069E3B–0x069F26`, `0x058FD0`,
`0x06A095–0x06A1B3`.

For the connected nonlocal action-1 attacker, the hit records
in scope use that attacker's nonlocal source ID. This holds for
the self heal and deferred feedback as well as hits against
another target. They can enter frame presentation if its
respective gates pass. The exact source comparison excludes
those hits from packet 7. A received record of the same type is
recognized by its pending-list origin at the earlier local
state pass, before it can join the shared presentation list.

## Limits

The contract covers valid client records and the named frame
consumers and socket send sites. Other hit types, the type-6
capture branch, renderer details beyond the named text and light
records, the packet-4 block transform, malformed or short packet
recovery, server behavior and the optional embedded-host accumulator
path remain outside this scope. The evidence is static.
