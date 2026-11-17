# Alpha connected action-1 melee candidates

**FINAL / STABLE / HELD**

Source is the original 32-bit Alpha `Cube.exe`, 3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs for image base `0x00400000`. This contract covers
a living nonlocal attacker using action byte 1 at `+0x68` in the world
step at `0x20C510`. The world byte `+0xB4` is nonzero and the attacker
pointer differs from the world's local creature pointer at `+0xB8`.
The supported domain is finite live state with representable arithmetic,
positive size and stat divisors, and nonnegative elapsed time. Arithmetic
described as binary32 rounds at each stage.

[alpha-connected-action-audio.md](alpha-connected-action-audio.md) covers
the preceding common sound sites. [alpha-connected-actor-turn.md](alpha-connected-actor-turn.md)
covers admission to the common action body and the later actor phases.
Action, species, record and sound numbers in this contract are decimal.

## 1. Saved clock window and earlier deferred feedback

For action 1 the ordinary saved bounds come from `0x212B43–0x212C05`.
With stable reads, let anticipation be the signed millisecond result
from `0x03CAA0` and middle be the result from `0x047310`, both requested
for the current action. The lower bound is anticipation plus half
middle minus 100, raised to anticipation if smaller. The upper bound
is anticipation plus half middle, lowered to anticipation plus middle
minus one if it reaches anticipation plus middle. Half middle truncates
toward zero. The native bound producer repeats the duration reads at
the sites above. Action 1 retains these ordinary bounds.

The action can adjust its live clock before the target window. With
byte `+0x13B8` clear, projected clock at least anticipation plus middle,
current clock below total duration and attacker roll at `+0x128` zero,
it clears combo count `+0x70` between two `0x047700` reads. It divides
the earlier binary32 result by the later one, multiplies by the
binary32 clock, and truncates toward zero to the signed clock field.
This uses the saved bounds computed earlier. Resource preparation then
runs. At live clock zero the action clears byte `+0x13C0` and recorded
resource `+0x13B4` before the cost check. A positive pending resource
`+0x144` is copied in full to `+0x13B4`, subtracted from mana, and
clears `+0x13B8`. This phase zeroes the pending resource and floors mana
at zero. The gate admits the live signed 32-bit clock at `+0x6C` only when
it is strictly above the saved lower bound and at or below the saved
upper bound (`0x213448–0x2134EE`, `0x2134F7–0x2137F1`,
`0x2137F7–0x213808`).

Two earlier passes can process existing deferred IDs before this gate.
At clock at or below current anticipation, copy combo count `+0x70` to
`+0x1314`, run deferred feedback, clear the struck and deferred sets
at `+0x11AC` and `+0x11B4`, and zero `+0x13B8`. At clock at or above
the saved upper bound, the expiry pass runs deferred feedback and clears
the deferred set. Action 1 uses that upper bound directly. Equality
can thus expire earlier deferred IDs and still admit the current target
window. The idle-action cleanup can also resolve leftover IDs on a
later turn (`0x213052–0x213200`, `0x2132CC–0x213448`,
`0x218597–0x218713`).

Deferred feedback visits saved signed 64-bit IDs in order. For each ID
absent from the struck set and resolving to a current creature, call
`0x03CF20` then append a type-4 hit with amount zero, attacker and
resolved target IDs, and the target's live fixed position. The benefit
helper tests current nonzero roll, class byte 4 and specialization
byte 1. If admitted it adds binary32 0.25 to target mana `+0x170` with
upper cap one, installs resident buff type 11 with duration 30,000 ms,
strength zero and resident source ID zero, and appends the target's
self-addressed buff event before the hit. The type-4 hit is appended
whether the benefit passes or fails. These feedback sites use no draw
(`0x2130C0–0x213199`, `0x213320–0x2133F3`, `0x03CF20–0x03CFE1`).

## 2. Candidate set, positions and geometry

The connected branch at `0x213B1E–0x213B25` jumps to `0x214084`.
For action 1 the continuation initializes the ordinary amount factor
to one and the per-turn success byte to zero before the iterator at
`0x214178`. The nonlocal continuation skips the intervening mode-zero
output and terrain span.

The iterator visits the live world creature map at world `+0x04`,
in ascending signed 64-bit key order. The high word compares as signed
32-bit and the low word as unsigned 32-bit for equal highs. Each node
contains a 64-bit key followed by the native 32-bit resident pointer.
The next candidate is the map successor. There is no distance sort.
The struck and deferred sets use the same signed 64-bit comparison
and retain one entry per value. Their candidate key is the resident
creature's ID at `+0x08`, read as 64 bits
(`0x20C555–0x20C558`, `0x214178–0x214217`, `0x214BB7–0x214BEB`,
`0x077DB0–0x077DC5`, `0x1C3EA0`, `0x07F000`, `0x053E39–0x053E83`).

For each slot, skip a null resident pointer, health at `+0x16C` at or
below zero, or an ID already in the attacker's struck set. Health is
binary32. These precede geometry and any relationship test.

Both actors supply their live fixed position at `+0x10`, three signed
64-bit components in units of 1/65,536 block. Size is the live
binary32 triple at `+0x80` and the attacker's third angle is the live
binary32 degree value at `+0x30`. Draw interpolation does not supply
these inputs. The action-1 base reach is twice attacker size X.
The range center starts at the attacker's live position and is
shifted by the yaw-rotated local vector with X and Z zero and Y equal
to attacker size X. The rotation uses the degree value times binary32
`0x3C8EFA35`, promotes the rounded radian input to binary64 for sine and
cosine, and rounds those results to binary32. For the ordinary identity
start the offset is minus sine times size X in X and cosine times
size X in Y. Each offset is multiplied by binary32 65,536 and truncated
toward zero into the fixed position
(`0x21380E–0x2138CF`, `0x2139C5–0x213A29`, `0x024610`, `0x088E50`,
`0x02C460`).

Candidates then pass the following gates in order.

1. Horizontal range uses target live X and Y relative to the shifted
   center. The radius is the binary32 sum of base reach and target size X
   times binary32 0.7. The two full fixed-point coordinate differences
   are squared separately and each square is divided by 65,536 in
   signed integer arithmetic before the sum. Compare that sum strictly
   below the fixed conversion of binary32 radius squared. The latter
   multiplies by binary32 65,536 and truncates toward zero
   (`0x214221–0x214334`, `0x087F80`, `0x1A7B50`).
2. The vertical gate takes the absolute value of the fixed Z difference
   from the center after conversion to binary32 blocks. It requires that
   value strictly below target size Z times 0.5, plus attacker size Z,
   plus base reach, with the three stages rounded in order
   (`0x21433A–0x2143C7`, `0x012200`, `0x0120F0`, `0x00CBA0`).
3. The sight test uses the attacker's unshifted live position copied at
   `0x21425A` and the target's live position. It uses limit 200 blocks
   and the enabled world-object mode (`0x2143CD–0x2143F1`).
   It converts the fixed difference to binary32 blocks and forms the
   rounded three-axis squared length. A result strictly below binary32
   0.0001 passes immediately. Above the squared limit it fails.
   Otherwise the binary64 square root rounded to binary32 defines the
   length and each component is divided by that length. The ray's
   returned distance must be at least binary32 length minus 0.01.
   The ray at `0x1A35D0` walks terrain with low-five-bit cell types 0
   and 2 passable. Its enabled object pass scans resident zone
   `+0x0C` entries of 32-bit types 3, 4, 5 and 8 with nonzero byte
   `+0x30`, using their fixed placements, rotation and dimensions.
   The detailed voxel and box intersection kernel is a dependency.
   Evidence is `0x19EE90–0x19F065`, `0x1A3774–0x1A3790` and
   `0x1A3A93–0x1A3CA3`.

## 3. Relationship, roll and deferral

After geometry, the relationship test at `0x1988A0` compares the live
attacker and target pointers and the controller bytes at `+0x60`.
An identical pointer is rejected. For distinct pointers, controller
kind 6 on either actor admits hostility, as does exactly one kind-1
actor. Two kind-1 actors admit hostility if their `0x044680` results
differ. For remaining pairs, mask `0x20` in the low status byte at
`+0x124` on either actor admits hostility. Remaining pairs are rejected.
This test uses neither an owner-ID lookup nor an early local-pointer
filter. The local-pointer protection is in the later hit helper.

The kind-1 classifier is true when the 16-bit field at `+0x7E` has
mask `0x1A00` clear and the 32-bit species at `+0x64` is one of
19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55, 56,
57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146,
or 147. Evidence is `0x044680–0x044759`.

A hostile candidate whose 32-bit roll at `+0x128` is nonzero inserts
its ID into `+0x11B4` and advances. This insertion emits no immediate
output and uses no draw. The struck set and `+0x13B8` receive no write
on this branch. A deferred ID can later reach the current hit path
when roll becomes zero, because the current eligibility lookup is
against the struck set. Geometry or relationship rejection and the
earlier null, health and struck skips likewise use no candidate draw
and insert neither ID (`0x2143F7–0x214441`, `0x214A0E–0x214A20`).

## 4. Live hit call inputs and pre-call writes

For a hostile, zero-roll target, take target live position minus
attacker live position and convert the fixed components to binary32
blocks. Set Z to zero and normalize the vector only if the squared
binary32 length strictly exceeds 0.01. The normalization divides one
by the rounded square root and multiplies each component by that
reciprocal. Then set Z to 0.25. Multiply each component first by the
rounded square root of attacker size X divided by target size X, then
by the current action-1 impulse factor from `0x044860`. That factor
is zero for attacker controller kinds 3 and 5, otherwise six divided
by `0x047700`'s binary32 result. This impulse uses the unshifted
positions, independent of the horizontal range center
(`0x214441–0x214529`, `0x0240F0`, `0x024B70`, `0x044860–0x04488A`,
`0x044908–0x044926`, `0x0449FA`).

The critical flag is true immediately if attack byte `+0x13C0` is
nonzero. Otherwise `0x0444A0` scans the attacker's resident buffs.
Presence of type 11 sets critical probability to one. Otherwise its
binary32 probability is the `0x03EA40` result plus binary32 meter
`+0x1190` times 0.15. One draw at `0x0444E5` is converted to binary32
and divided by 32,767, and the flag is true only if the probability
strictly exceeds that value. The type-11 route still draws. The
resident-type probability test here does not read duration
(`0x2144A8–0x2144D6`, `0x0444A0–0x04451E`).

The action-1 amount uses the action factor one from `0x03EC90`
multiplied by the binary32 attack stat returned by `0x03DC60`.
The multiplier for recorded resource `+0x13B4` is one plus five
times the binary32 squared resource, rounded at each operation.
Multiply the attack product by that multiplier, then multiply by
1.25 minus the binary32 result of draw times 0.5 divided by 32,767.
The amount draw is at `0x2145C7`, after the optional critical draw.
The jitter path adds binary32 positive zero before that multiplication.
Finally multiply amount by two if critical. The attack stat and
critical-stat equipment formulas are named dependencies
(`0x214561–0x214600`, `0x21467B–0x21469C`, `0x03ECE8`).

After the amount calculation, remember whether the struck set was
empty and attacker signed stun at `+0x12C` was at most zero. Then
insert the target ID in the struck set and set byte `+0x13B8` to one,
before calling `0x196D30` at `0x21476B`. The call receives the live
attacker and target, original amount and critical flag, the saved
resource indicator and live recorded resource value, attacker live
position and impulse, and the current output collector. For action 1
the resource indicator means recorded resource was strictly positive
at `0x213625`. The call's actions-30-through-33 flag is false, the
action argument is 1, the following argument is zero, and the final
guard-enabled flag is one. Evidence is `0x21469C–0x214772`.

## 5. Ordered defensive result branches

The hit helper first retests positive target health and in connected
mode rejects a target pointer equal to world `+0xB8`. This returns
false with no hit-helper draw or output. The preceding caller draws,
struck insertion and byte write already happened
(`0x196D7F–0x196DAE`, `0x197014`). For the remaining action-1 targets,
the helper tests the following routes in order.

### 5.1 Controller-kind-1 protection

Target kind 1 with low-byte mask `0x80` at `+0x124` or contact-byte
mask `0x20` at `+0x5C` admits this route. Append sound 24 at the
target's live position with volume one and pitch one plus binary32
draw times 0.3 divided by 32,767. The draw is `0x196EF9`. Then append
type-3 hit with the caller amount and full caller impulse, zero
critical and stun, both live actor IDs and the target's live position.
Return false. The mode-zero application after the record is skipped
(`0x196EC1–0x197016`).

### 5.2 Active guard

The guard-enabled argument reaches `0x044590`. It requires signed
`+0x130` at most zero and target action 8, 71, 72 or 98, or target
action 59 or 63 with class and specialization both 1. Guard further
requires binary32 target `+0x174` strictly positive.

Take incoming amount divided by the guard-stat result at `0x03E190`.
Add the old `+0x174` separately to target mana `+0x170`, capped at one,
and to pending resource `+0x144`, capped at the newly stored mana.
Subtract the quotient from the old `+0x174` and floor the result at
minus one. Append a type-1 hit with original caller amount, half of
each caller impulse component, zero critical and stun, and the same
live ID and position sources as above. Next append sound 24 at the
target's live position with the same pitch and volume as protection,
using its own draw at `0x1971C0`. Return false
(`0x19701B–0x197230`, `0x044590–0x0445CB`).

### 5.3 Resident type-6 ward

After guard misses, sum the binary32 strengths of target resident
buff type 6 in list order. Use the signed attacker combo count
`+0x70` divided in binary32 by `0x044CB0`'s current positive result
to obtain combo fraction. Subtract that fraction, upper-capped at one,
from one to obtain the defense multiplier. The effective ward is the
type-6 sum times that multiplier.

For a type-3 item at attacker `+0xAA8`, `0x044CB0` truncates the
binary32 product of attacker `+0x17C` and multiplier 60 for subtype 6,
80 for subtype 8, 20 for subtypes 11, 12, 15, 16 and 17, and 50 for
all other subtypes. For other items it uses 20 if low byte `+0x7E`
has mask `0x10` set, and 50 otherwise
(`0x044CB0–0x044D28`, table `0x044D2C`).

A strictly positive effective ward appends type-5 hit with original
caller amount, half of each caller impulse component, zero critical
and stun, and the same live IDs and position. It then appends sound
91 at the target's live position with volume one and pitch one plus
binary32 draw times 0.3 divided by 32,767. This draw is `0x197478`.
Subtract the effective ward from caller amount after those appends.
When the remainder is at most zero the helper returns true here.
The connected branch skips application of the type-5 record. If the
remainder is positive, continue using it. With no positive effective
ward, continue using the caller amount
(`0x197235–0x19750B`, `0x1986D5`).

## 6. Ordinary resolution and dependent equipment effects

Multiply the target physical-defense stat from `0x03CFF0` by the
defense multiplier. Subtract the product from the current amount
and floor it at zero. The helper reads the attacker combo fraction again
for equipment work using the same divisor. This latter fraction is
not upper-capped. The known defense and equipment routes occur before
ordinary hit sound (`0x19750B–0x197584`).

For action 1 the weapon collector at `0x03DF60` first picks item
`+0xAA8` alone if it has category 3 and subtype 6 or 7. Otherwise
the same pair at `+0x990` selects that item alone. Otherwise collect
`+0xAA8` if its category is 3 and then `+0x990` if its category is 3
and subtype is 12. This determines weapon effect order and the
first item for ordinary sound selection (`0x03DFA9–0x03E0D6`).

With combo fraction strictly above 0.25, each collected weapon is
visited for its enhancement effects at `0x19A800`. It scans the
signed count at item `+0x114` in recorded order with eight-byte
entries starting at item `+0x14`. Entry byte `+0x03` at least 128
appends a visual effect to collector `+0x10` at the target's live
fixed position. This path uses the caller impulse direction
normalized using all three components when its length is positive.
The horizontal components are then multiplied by four and the
effect Z component is assigned five. The resource flag doubles the
three effect components and adds ten to its base integer count,
which truncates one plus three times combo fraction. The effect
color selector `0x0C7250` uses the entry type and half the combo
fraction. This pass uses no direct random draw
(`0x19758A–0x197687`, `0x19A800–0x19A993`).

The subsequent resource enhancement path tests the passed resource
flag and a positive combo value for a nonempty weapon list. It
walks each weapon's same enhancement entries. Weight is binary32
0.1 divided by weapon count. The level term `0x03CA60` is one minus
the reciprocal of one plus the signed level minus one times
binary32 0.05, with stages rounded in that order. Type 128 adds
level term times weight times combo fraction to amount. Type 129
adds half of that contribution to amount and twice that contribution
to a running attacker heal. Types 130 and 131 add to amount using
one quarter of weight before multiplying it by the level term and
combo fraction. Each type-130 entry adds combo fraction times 500
to the target signed clock `+0x134`, and type 131 does the same
to the attacker signed clock `+0x138`. Each write converts the old
clock to binary32, adds, and truncates toward zero. The two clocks
are capped at 5,000 after the weapon walk. The resource branch and
entry selectors are `0x1976D8–0x19798D` and table `0x1986FC`.

If this path has positive accumulated healing, it appends a
self-addressed ordinary hit with the negative heal amount and the
attacker's live position, then adds the positive amount directly
to attacker health `+0x16C`. This write occurs in connected mode
and has no cap at this site. It precedes the following normal target
hit and uses no direct draw (`0x19798D–0x197A71`).

## 7. Ordinary result sounds, stun and append

The continuing action-1 path writes binary32 one to target `+0x1184`.
It then uses one draw at `0x197D1B` for ordinary hit sound at the
target's live position, volume one. The base pitch is binary32 0.9
plus binary32 draw times 0.2 divided by 32,767. Add another binary32
0.1 for first weapon subtype 0 or 1. The collector's first weapon
subtype or empty result selects the sound as follows
(`0x197AC1–0x197B09`, `0x197C5A–0x197E36`, tables
`0x19870C`, `0x198728`, `0x19873C` and `0x198758`).

| First weapon subtype | Ordinary sound | Critical sound |
| --- | --- | --- |
| 0, 1, 15, 16 | 1 | 2 |
| 3, 4 | 7 | 8 |
| 5 | 3 | 4 |
| 6, 7, 8 | 9 | 10 |
| All other values or empty | 5 | 6 |

The ordinary record uses the target's live fixed position and the
caller impulse. Target resident buff type 1 multiplies the current
amount by strength once per matching record in list order. There is
no duration test at that scan. The record carries source and target
64-bit IDs at `+0x00` and `+0x08`, binary32 amount at `+0x10`,
critical byte at `+0x14`, initial stun zero at `+0x18`, position
at `+0x20`, impulse at `+0x38`, the passed resource indicator at
`+0x44`, and zero type and auxiliary bytes at `+0x45` and `+0x46`.
Evidence is `0x197FA8–0x1980BF`. The preceding velocity comparison
forms the maximum of old and proposed squared speed and would write
only if the same old squared speed exceeds that maximum. It makes
no velocity write for the finite supported state (`0x197E5E–0x197FA8`).

With critical true and target controller kind different from 6, a
second sound route first draws at `0x198258`. Pitch is binary32 one
plus binary32 draw times 0.1 divided by 32,767, volume one, at the
target's live position. Species is the 32-bit field at target
`+0x64`. This table shows the append cases. An unlisted species
still consumes the draw when the route is admitted
(`0x1981CB–0x1983EC`, tables `0x19876C` and `0x1987C4`).

| Species | Sound | Species | Sound |
| --- | --- | --- | --- |
| 0 | 60 | 1 | 61 |
| 2, 43 | 62 | 3, 45 | 63 |
| 4 | 64 | 5 | 65 |
| 7 | 66 | 8 | 67 |
| 9 | 68 | 10 | 69 |
| 11 | 70 | 12 | 71 |
| 15 | 72 | 16 | 73 |
| 13 | 74 | 14 | 75 |
| 119 | 76 | 46, 108, 109, 114, 115 | 77 |
| 48, 51, 87 | 78 | 37–40 | 79 |
| 96 | 80 | | |

After that optional draw, a stun roll is considered only when target
signed `+0x12C` is strictly below minus 3,000 and target controller
kind differs from 6. It starts from the caller's recorded resource.
Multiply it by binary32 0.15 when the target species passes
`0x0445D0`. Multiply by binary32 0.1 when the attacker fails the
same classifier and its controller kind differs from zero. The
classifier admits species 101, 107, 108, 109, 114, 115, 116, 117,
118 and 119. Target action 84 replaces the adjusted quantity with
one. Form the two actors' level terms via `0x03CA60`, subtract the
target term from the attacker term, multiply by binary32 ten, and
use that result as the exponent for binary64 power with base two.
Round the result to binary32 and multiply by the adjusted quantity.
Cap at one and multiply by binary32 0.9. Any target resident buff
type 1 then forces probability to zero. The admitted route still
draws at `0x198545` even for zero probability. Compare probability
strictly greater than binary32 draw divided by 32,767
(`0x1983EC–0x198563`, `0x0445D0–0x04460A`).

On a passing stun roll fill the record's stun duration from
`0x044FF0`, assign the record's impulse Z ten, and append sound 23
at the target's live position with pitch and volume one. Duration is
1,500 ms for target controller kind zero. Otherwise it is 3,000
minus the integer quotient of the target byte `+0x1A8` times 1,500
divided by four. At this site the stun duration lives in the outgoing
record. Append the ordinary type-0 target hit
afterward, with any prior protective or auxiliary outputs already
in the collector. The connected return is true and bypasses the
mode-zero hit application (`0x198569–0x1986D5`, `0x044FF0–0x04501A`).

## 8. Caller result and scope limits

A false result advances action 1 without a combo reward or resource
meter gain. The struck ID and byte `+0x13B8` stay recorded. A true
result increments combo count `+0x70` and clears its timer `+0x74`
only when the remembered pre-insertion condition was true. Between
those writes it calls `0x0453B0`. A successful hit can therefore miss
the combo reward when an earlier candidate with a lower map key
inserted a struck ID and subsequently returned false.

Independently on true, class 4 specialization 0 with the saved
resource indicator adds the recorded resource in binary32 to meter
`+0x1190`, capped at one. This occurs before the next candidate.
There is no success-driven mana draw for the connected nonlocal
attacker. The side check at `0x214906–0x21491B` skips the sites at
`0x214949` and `0x2149B4`. The pass records that a result returned
true and advances. Action 1 never takes the action-32/33 alternate
route at `0x214A26`. The post-pass paths at `0x214BF1–0x214C2D`
rejoin the common bookkeeping without further candidate draws.
The ordinary combo timeout increment at `0x211F56` is skipped for
the connected nonlocal actor. Evidence is `0x214770–0x214772`,
`0x214899–0x214A20`, `0x214BB1–0x214C2D` and `0x211F2F–0x211F59`.

The window and ordinary hit routes above have static evidence from
the supplied client image. Full equipment stat formulas, the ray
intersection kernel, visual effect appearance and output consumption
remain dependencies. Dedicated authority, local-attacker reward
draws, other action-specific call arguments and per-action hit
branches such as 17, 28 and 30–33 are outside this action-1 audit.
