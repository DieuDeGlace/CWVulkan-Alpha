# Alpha connected-client walk phases, facing and attachment

Status **FINAL / STABLE / HELD**

## Scope

The source is original 32-bit Alpha `binary/Cube.exe`, 3,885,568 bytes,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
image base `0x400000`. Code addresses are client RVAs. Evidence comes from
that image and the matching original world listing
`decompiled/client/assembly-functions/0060c510.asm`. The world step starts
at `0x20C510`. The connected path has world byte `+0xB4` nonzero. A
connected nonlocal actor has a pointer different from world `+0xB8`.
The main per-actor body below is reached after the actor's ordered
positive-health check at `0x211AD1–0x211AE1`. The separately scoped later
attachment traversal can also visit actors that skipped that body.

The step duration is signed integer milliseconds. The world step stores
the binary32 conversion of those milliseconds and separately multiplies it
by binary32 `0x3A83126F` to obtain seconds. Unless a wider stage is named,
each floating operation below rounds independently to binary32. This
covers finite valid values, nonnegative durations and in-range integer
arithmetic and conversion. Action numbers and sound IDs are decimal.
Evidence is `0x20C549–0x20C54F` and `0x20C83D–0x20C862`.

## 1. Input fields and turn order

| Actor offset | Representation and use |
| --- | --- |
| `+0x10` | Three signed 64-bit fixed coordinates, 65,536 units per block. |
| `+0x28` | Three binary32 live orientation components in degrees, with yaw at `+0x30`. |
| `+0x34`, `+0x40` | Binary32 live velocity and movement intent, three components each. |
| `+0x58` | Binary32 head pitch in degrees. |
| `+0x5C` | Contact flags. Mask `0x01` is ground, `0x02` water and `0x04` wall contact. |
| `+0x60`, `+0x68` | One-byte controller kind and current action. |
| `+0x6C` | Signed 32-bit live action clock. |
| `+0x7E` | Appearance flags, including carry of the walk phase off ground when mask `0x0002` is set and the speed gate is met. |
| `+0x80`, `+0x88` | Binary32 collision box X and Z dimensions. |
| `+0x124` | 16-bit mode mask. The traced flags are climb `0x0001`, face aim `0x0004`, glider `0x0010` and lantern `0x0200`. |
| `+0x128`, `+0x12C` | Signed 32-bit roll and stun durations. |
| `+0x1180`, `+0x1184`, `+0x1188`, `+0x118C` | Binary32 landing bob, hit flash, walk amplitude and walk phase. |
| `+0x11A0` | Three binary32 wall-normal components on the climbing-facing path. |
| `+0x11C0`, `+0x198` | Distinct 64-bit creature links used by the two attachment paths. |
| `+0x1318`, `+0x138C` | Signed 32-bit pre-decrement roll snapshot and binary32 three-component drawn aim. |

The action motion phase first prepares the flag used by later yaw. After
physics and contact changes, the ordinary creature-link check either
copies another creature's phase and amplitude or enters the subject's own
walk update. A ground footfall uses the phase just updated by that
subject. Equipment mask checks and amplitude, bob and flash relaxations
follow. Climbing orientation or the normal facing branch then runs
before the roll sound, roll snapshot and decrement. The per-actor
carried-pet pose copy follows those stages, before the actor's drawn-aim
update. A later map traversal can repeat that carried-pet copy.
The walk, facing and roll snapshot regions admit the connected
nonlocal actor. The footfall and roll sound sites separately test the
presence of the local creature. Evidence is `0x21A266–0x21A74D`,
`0x21E9B5–0x21F6DA`, `0x21F968–0x21FDFC` and `0x220E51–0x221136`.

## 2. Action-driven facing flag

The per-turn flag starts clear at `0x21A269`, after the ordinary clock
update and the roll action-cancellation gate. These predicates read the
surviving action and live signed clock. Anticipation is returned by
`0x03CAA0`, middle duration by `0x047310` and total duration by
`0x03D1A0`. The native flag-setting sites are as follows.

| Actions | Clock predicate at the setting site | Evidence |
| --- | --- | --- |
| 68, 69, 93 | Strictly above anticipation and strictly below anticipation plus three quarters of middle, with truncation toward zero of the middle fraction. | `0x21A295–0x21A381` |
| 77 | Strictly above anticipation and strictly below anticipation plus middle. | `0x21A388–0x21A463` |
| 78 | Same interval as 77. | `0x21A470–0x21A54B` |
| 76 | Same interval as 77. | `0x21A558–0x21A633` |
| 71 | Strictly below total duration. This branch sets speed limit to zero. | `0x21A640–0x21A65D` |
| 72 | Strictly below total duration. This branch changes live yaw. | `0x21A664–0x21A690` |
| 68, 69, 93 | A second test sets the flag when clock is strictly above anticipation and strictly below total duration. Otherwise this test preserves its earlier value. | `0x21A6AF–0x21A6E7` |
| 57, 74 | Clock at least anticipation and strictly below total duration. The native branch queries and compares anticipation a second time. | `0x21A703–0x21A746` |

The initial speed-zero branch for 68, 69 and 93 at or below anticipation
leaves the flag clear. The above sites and the reset are the direct
writes found between reset and the two yaw reads. The flag persists
through the intervening movement, collision and walk-cycle work and is
read at `0x21F2F1` and `0x21F543`. Its value tracks these action windows
even in the action-71 stop branch, and also survives a later link copy
that clears the movement vectors. The original-image literal local-slot
check was restricted to this live interval.

## 3. Ordinary link and walk production

The first link check, before walking, looks up the current 64-bit value
at `+0x11C0` in the world creature map. A missing actor preserves the
link and enters the usual walk update. A resolved actor with health at
most zero clears the two-word link and enters that update. For a
resolved actor with positive health, subject controller kind 5 enters
the usual walk update and keeps the link. The remaining subjects copy
the linked actor's fields in this order.

1. Set subject live position to the linked actor's position plus a Z
   offset computed from binary32 `1.5 − linked landing bob`. Multiply
   the rounded offset by binary32 65,536 and truncate to signed 64 bits
   before adding in fixed point.
2. Clear live velocity to positive zero in all three components. Form
   live orientation from the linked orientation plus the binary32
   component increments (20, 0, 0). Clear movement intent to positive
   zero in all three components.
3. Store the linked amplitude multiplied by binary32 0.5, then copy
   the four-byte phase. Continue at the equipment and relaxation
   tail, skipping the subject's own phase increments and footfall
   event for this visit.

This copy uses the linked actor's currently stored values and leaves
the subject's own landing bob for the common bob relaxation. Evidence
is `0x21E9B5–0x21E9EC`, `0x21EAD7–0x21EC03`, lookup `0x02F000` and
fixed conversion `0x02C460`.

The usual update uses contact bits available after collision and the
later contact work. It considers an actor eligible when grounded, in
water, flying by appearance mask `0x0002`, or climbing with both mode
`0x0001` and contact `0x04`. For an eligible actor, the binary32
three-dimensional squared velocity must be strictly greater than 0.5
and the current action must differ from 107 to take an increment
branch. The squared norm adds the rounded X and Y squares and then the
rounded Z square. Horizontal speed is the square root of the rounded
sum of horizontal squares. The square-root helper promotes the
binary32 input to binary64 and rounds the root to binary32.

In the phase formulas below the size factor is the native square root
of binary32 `0.8 / box X`, with binary32 0.8 bits `0x3F4CCCCD`. The
first two items in each product are multiplied first, then the next
factor is applied in the order displayed. Each increment is added to
its actor field after the multiplication stages.

| Branch, tested in order | Phase increment stages | Amplitude increment |
| --- | --- | --- |
| Climb mode with wall contact | Full three-dimensional speed × seconds × 6 × size factor. | Seconds × binary32 0.2. |
| Ground and slow action window below | Binary32 milliseconds × `0x38D1B718` × horizontal speed × 1.5 × size factor. | Seconds. |
| Other ground | Horizontal speed × seconds × 1.5 × size factor. On action 79, multiply the rounded result by binary32 2 before adding. | Seconds. |
| Eligible, otherwise off ground | Binary32 milliseconds × `0x3B03126F` × horizontal speed × 1.5 × size factor. | Seconds. |

The slow ground branch admits action 48 at every clock, action 54 at a
signed clock strictly below total duration, and actions 6, 7, 10, 17,
18, 19 and 20 at a signed clock strictly below anticipation. Equality
to the corresponding limit selects the ordinary ground rate. The
clock at this site already includes earlier mutations and any clock
increment. The native ground branch saves the old binary32 phase
before computing horizontal speed, writes the new phase, then adds
seconds to amplitude before checking for a footfall.

If the eligibility, velocity or action check declines the increment,
ground contact, water contact or the climb-mode-plus-wall condition
preserves both fields up to the common tail. With all three absent,
write positive zero to phase and add size factor multiplied by seconds
to amplitude before the tail. The active climbing branch takes
priority even if ground contact is also set. Only the two ground
branches reach the walking footfall test. Evidence is
`0x21912D–0x219151`, `0x21E9EC–0x21EAD2`, `0x21EC08–0x21ED8F`,
`0x21EF0B–0x21F008` and helpers `0x023EE0`, `0x023F20`,
`0x024860` and `0x024B70`. The slow and off-ground constants are at
`0x31E1D4` and `0x2FCD98` respectively.

## 4. The pre-footfall cycle

After the ground updates above, the sound requires a nonnull local
pointer at world `+0xB8`, a zero **pre-decrement** roll duration, current
water flag clear and horizontal speed strictly above binary32 5.
Promote the old and new stored binary32 phases separately to binary64.
Divide each by binary64 π and truncate each quotient toward zero to
signed 32 bits. Append one sound only when the new quotient is larger
than the old one. This site has no multi-interval repeat.

The support query reads live fixed X and Y at the actor position.
For Z subtract fixed-point half-height and then fixed-point one half
block from the live Z. Compute the half-height as binary32 box Z
times 0.5. Each lowered offset is independently multiplied by
binary32 65,536 and truncated to signed 64 bits before subtraction.
The world query at `0x02F860` converts each nonnegative fixed input
by signed division by 65,536 with truncation toward zero. For a negative
input it subtracts one block first, then divides the same way. An
exactly aligned input of −65,536 therefore selects block coordinate
−2. The low five bits of the sampled block type byte select the sound.

Type 3 takes one `MSVCR110.rand` draw, selects sound 33 plus the
draw's remainder modulo 3, and uses pitch 1. Every other type
takes one draw and uses sound 32. For the latter pitch, convert the
draw to binary32, multiply by binary32 0.2, divide by 32,767 and
add 1. Both records use the actor's current position and volume 1.
The type-3 draw is at `0x21EE7E` and the other at `0x21EEAB`.
Evidence is `0x21D842–0x21D8DA` and `0x21DAF1–0x21DB6B` for live Y
and X aliases, `0x21ED8F–0x21EF06` for the gate and append, and
`0x012220`, `0x02F860`, `0x034C10` and `0x0864C0` for the helpers.

For example, use 16 ms, box X equal to binary32 0.8, speed 8 on the
horizontal axis, no vertical motion, and old phase 3. The ordinary
ground branch writes approximately 3.192. This crosses the first
π quotient boundary. The slow ground branch instead writes
approximately 3.0192 and stays below that boundary. These values are
read before the amplitude tail.

## 5. Equipment, orientation and the roll boundary

After normal walk or the ordinary link copy, the equipment checks
clear mode `0x0010` unless bytes `+0xF08` and `+0xF09` are 23 and 0,
then clear mode `0x0200` unless byte `+0xDF0` is 24. Cap amplitude at
one if above one and approach zero at rate bits `0x3BA3D70A`. Next
approach zero for landing bob at rates `0x3C23D70A` when negative and
`0x3CA3D70A` otherwise. Finally approach zero for hit flash at rate
`0x3BF5C28F`. An approach step rounds the target-minus-current
difference, its product with the rate, and the sum with current
separately. The native helper repeats this once per elapsed integer
millisecond. Evidence is `0x21F008–0x21F0F1` and `0x011D40`.

For climb mode and wall contact with glider mode clear, head pitch
approaches 45 degrees at rate bits `0x3BA3D70A`. The yaw target comes
from the negated and normalized three-component wall normal at
`+0x11A0`. The routine turns toward that heading through the angle
helper and the same approach rate. This branch rejoins at the roll
stage. It bypasses the normal free-facing routes and their flag reads.
Evidence is `0x21F0F1–0x21F262`, `0x068E20`, `0x0240F0` and
`0x023F70`. The supported wall-normal domain is nonzero.

On the other branch, head pitch first multiplies by the binary32
return of the precise exponential of the binary32 product of
milliseconds and binary32 −0.01. The exponential is evaluated in
binary64 on that rounded product. Then the facing branch makes
these choices.

- When mode `0x0004` is set and signed stun is at most zero, use the
  drawn aim already resident at `+0x138C`. Yaw is eligible when its
  horizontal squared length is strictly positive and the action-driven
  flag is clear. Normalize the horizontal pair, obtain its heading,
  and use the angle helper with weight 1, then approach the result
  with rate bits `0x3DCCCCCD` for the step. The head-pitch test is
  separate. A strictly positive three-dimensional squared aim length
  permits normalization and the arcsine of normalized Z. Convert
  the angle to degrees, use the angle helper with weight 1, then
  approach it with rate bits `0x3BA3D70A`. This head-pitch arm reads
  neither the flag nor the roll timer.
- When face aim is clear or signed stun is positive, the movement-yaw
  test requires the **current roll duration exactly zero**, horizontal
  live velocity squared length strictly above binary32 `0x3CA3D70A`,
  the three-component velocity and intent dot product strictly above
  zero, and a clear flag. The dot product forms each binary32 product
  and adds X and Y before Z. Yaw is set directly to the heading of
  normalized horizontal velocity. Evidence is `0x21F4DB–0x21F62B`.

For yaw heading, the arccosine argument is normalized Y clamped to
[−1, 1]. Negate the binary32 radian result if normalized X is positive.
For degree conversion, promote to binary64, divide by binary64 π,
multiply by binary64 180, then round to binary32. An actor's drawn-aim
write in this turn follows these reads. Evidence for the exponential
and aim path is `0x21F262–0x21F4D6`, `0x222D40`, `0x024090`,
`0x0240F0`, `0x024B30` and `0x0C14F0`.

The angle helper `0x023F70` divides the current and desired degree
values by binary32 180. Each quotient is promoted and multiplied by
binary64 π, rounded to binary32, and promoted again for precise
sine and cosine. Round each trigonometric result to binary32 before
forming the sine difference in binary64. This difference is sine of
the desired angle times cosine of the current angle, minus cosine
of the desired angle times sine of the current angle. Round the
difference to binary32 before clamping it to [−1, 1] and evaluating
arcsine in binary64. Round that result to binary32. At weight 1 the
helper multiplies it by binary64 180 divided by π, rounds the degree
delta to binary32, then adds it to the current degree value. None of
these traced facing operations draws from the random import.

The two general yaw paths precede the roll sound and subtraction.
The sound is 26 when a local pointer exists, old signed roll is
strictly above 500, and old roll minus the integer step is at most
500. It uses the subject's current position, unit pitch and volume,
and no draw. Then copy old roll to `+0x1318`, subtract elapsed if
roll is nonzero, and clamp negative roll to zero. Thus a roll that
reaches zero here still blocked the movement-yaw and footfall tests
earlier in this same turn. Evidence is `0x21F63D–0x21F6DA`.

## 6. Carried-pet copies after the roll boundary

The connected mode path from `0x21F6DA` bypasses the separate
world-byte-zero object transform and reaches the displayed-pose
branch at `0x21F968`. The carried-pet arm checks controller kind 5,
looks up the actor named by `+0x198`, and requires that actor's current
action to be 106. The copy reads the owner's then-current fields. A
later map traversal repeats those tests after the main actor loop
and re-reads the owner fields. The first copy follows this actor's
earlier walk, equipment, facing and roll stages and precedes the
drawn-aim write. The later copy does not run those stages again and
can visit a pet whose main visit exited for nonpositive health. It
follows a separate `+0x11D0` link check.

Both copies assign the owner's live and displayed positions to the
pet, each with the Z offset formed from binary32 pet box Z times
0.5 minus binary32 owner box Z times 0.5, then plus binary32 0.01.
For each position, multiply the resulting offset by binary32 65,536
and truncate to signed 64 bits before adding. The live orientation
and displayed body rotation take the owner's corresponding vectors.
Landing bob, amplitude and phase take the owner's corresponding
four-byte values. Velocity and movement intent each become three
binary32 positive zeroes. Consequently the phase and amplitude may
be replaced after this actor already passed its own footfall and
relaxation sites. There is no second footfall test in either copy.

The copy gates read the owner action and do not add an owner-health
predicate. The copied fields exclude the pet's head pitch, roll
duration and roll snapshot. Evidence is `0x21F968–0x21FB75`,
`0x21FD4E–0x21FD54` and `0x220E51–0x221136` with fixed conversion at
`0x02C460`. Whether the second copy is taken depends on the live
role and owner action after the full main traversal.

## Limits

This audit covers the original client's phase and amplitude producers,
the pre-footfall phase boundary, the direct action-driven flag,
the facing and roll order and the named creature links. The complete
construction of wall normals and contact flags, the production and
replication of link fields, later world-object transforms, and sound
playback remain outside its scope. Evidence is static.
