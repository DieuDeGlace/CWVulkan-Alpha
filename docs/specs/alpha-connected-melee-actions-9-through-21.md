# Alpha connected melee actions 9 through 21

**FINAL / STABLE / HELD**

Supported image is the original 32-bit Alpha `binary/Cube.exe`, 3,885,568
bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are client RVAs at image base `0x00400000`. Selector byte
ordinals, pointers and numeric literals were read directly from that PE.
Action IDs, sound IDs and byte-table ordinals are decimal.
The domain is finite native state with
nonnegative elapsed milliseconds, representable intermediates and positive
divisors, including a positive signed pulse period for action 11.

This covers the common melee body of a living actor at `0x20C510` with
world mode byte `+0xB4` nonzero and attacker pointer different from world
local pointer `+0xB8`. It follows the body through ordered candidate results
and the immediately relevant resident buff pass. The shared ray,
relationship, protection and enhancement details are in the held
[connected action-1 candidate contract](alpha-connected-melee-candidates.md).

## 1. Dispatch and duration selectors

The outer dispatch zero-extends live action byte `+0x68`, subtracts one
and tests the result against unsigned maximum 109. An out-of-range index
goes to `0x215EC0`. For actions 9 through 21 the thirteen bytes at
`0x222CD4–0x222CE0` are all zero, selecting pointer ordinal zero at
`0x222C84`, which leads to `0x212357`. World mode nonzero goes directly
to `0x212378`, bypassing the mode-zero control-kind and local-pointer
checks (`0x211E04–0x211E1B`, `0x212357–0x212378`).

At live signed clock `+0x6C` equal to zero, actions 17 and 20 set signed
roll `+0x128` to 600 ms if class byte `+0x140` is 4 and specialization
byte `+0x141` is 1. Those action compares at `0x21238A` and `0x212394`
join the same store at `0x21239F`. This write runs before the window and
resource work and needs no local ownership.

Anticipation A and middle M take the live action when the caller supplies
minus one. The total at `0x03D1A0` takes the live action directly and adds
its own anticipation, middle and recovery R results. Each scaled leg
samples speed through `0x047700`, rounds speed times binary32 actor
`+0x17C` to form its divisor, divides the numerator in binary32 and
truncates toward zero to signed milliseconds. The table gives the
numerator for a scaled leg. Fixed values are signed integer milliseconds
and take no speed sample in that leg.

| Action | A | M | R | A / M / R byte ordinals |
| --- | --- | --- | --- | --- |
| 9 | 300 | 200 | 500 | 1 / 1 / 15 |
| 10 | 100 | fixed 200 | fixed 600 | 5 / 5 / 3 |
| 11 | fixed 0 | 300 | 100 | 0 / 6 / 4 |
| 12 | 200 | 400 | 500 | 6 / 3 / 15 |
| 13 | 200 | 200 | 500 | 7 / 7 / 15 |
| 14 | 100 | 200 | 200 | 8 / 7 / 2 |
| 15 | 200 | 200 | 400 | 9 / 7 / 5 |
| 16 | 200 | 400 | 500 | 6 / 3 / 15 |
| 17 | 100 | 400 | 500 | 10 / 3 / 15 |
| 18 | 100 | 150 | 200 | 10 / 8 / 2 |
| 19 | 100 | 150 | 500 | 10 / 8 / 15 |
| 20 | 100 | 400 | 500 | 10 / 9 / 15 |
| 21 | 100 | 400 | 500 | 10 / 9 / 15 |

The PE selectors below use the unsigned action itself as the byte index,
with zero subtracted. The listed pointer targets are RVAs converted from
the original little-endian preferred virtual addresses.

| Leg and reader | Unsigned maximum | Byte table | Pointer table | Out-of-range target |
| --- | --- | --- | --- | --- |
| A-only at `0x03CAA0` | 105 | `0x03CEB0` | `0x03CE24` | `0x03CDF8` |
| M-only at `0x047310` | 104 | `0x047628` | `0x0475A8` | `0x04757C` |
| Total A at `0x03D1A9` | 105 | `0x03DA54` | `0x03D9C8` | `0x03D4EB` |
| Total M at `0x03D510` | 104 | `0x03DB40` | `0x03DAC0` | `0x03D7DD` |
| Total R at `0x03D804` | 104 | `0x03DBEC` | `0x03DBAC` | `0x03D994` |

- A ordinals 0, 1 and 5 through 10 select `0x03CADC`, `0x03CC9E`,
  `0x03CC6A`, `0x03CCEC`, `0x03CD3A`, `0x03CD54`, `0x03CD6E` and
  `0x03CD20`, respectively. Their total-A counterparts are `0x03D1D8`,
  `0x03D39A`, `0x03D366`, `0x03D3E8`, `0x03D436`, `0x03D450`,
  `0x03D46A` and `0x03D41C`.
- M ordinals 1, 3 and 5 through 9 select `0x0473BD`, `0x0473F1`,
  `0x0473A1`, `0x047528`, `0x047425`, `0x04736D` and `0x04740B`,
  respectively. Their total-M counterparts are `0x03D5B8`, `0x03D5F0`,
  `0x03D59A`, `0x03D74E`, `0x03D628`, `0x03D562` and `0x03D60C`.
- R ordinals 2, 3, 4, 5 and 15 select `0x03D8CB`, `0x03D89F`,
  `0x03D84B`, `0x03D8E7` and `0x03D994`, respectively. Ordinal 15
  reaches the fallback address by an in-range table entry.

## 2. Earlier sounds, saved timing and expiry

Before the candidate walk, the body reads earlier clock from `+0x6C`.
Projected clock is that value plus the signed frame step. These sounds
use the attacker's live fixed position converted to binary32 blocks,
volume one and binary32 pitch stages.

1. At earlier clock zero, `0x044230` admits category 3 at `+0xAA8` with
   subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18. On that route draw at
   `0x212442` and append sound 17 with pitch one plus draw times 0.1
   divided by 32,767 (`0x2123F3–0x212485`).
2. Earlier clock at or below A and projected clock strictly above A
   consumes a draw at `0x212738`. Base pitch is 0.9 plus draw times 0.2
   divided by 32,767. Action 10 appends sound 16 and action 11 appends
   sound 48, both at that pitch. The other in-scope actions append
   sound 15 after multiplying that pitch by 1.5 and multiplying
   again by 0.5 if `0x044230` is true (`0x2126F3–0x212762`,
   `0x21276A–0x212797`, `0x21297B–0x212A45`).

The ordinary saved lower bound is A plus signed truncation of M divided
by two, minus 100, raised to A when smaller. The upper is A plus signed
truncation of M divided by two, capped at A plus M minus one. Actions 9,
10 and 12 through 21 retain both bounds at `0x212B43–0x212CDE`.

Action 11 takes the pulse path through `0x212CD4–0x212CD6`. That path
sets both bounds to A if earlier clock is below A or at least A plus M
(`0x212CEC–0x212D21`, `0x213037–0x213052`). Inside, period is M divided
by three with signed truncation. The lower is A plus the period times the
signed truncated quotient of elapsed time since A divided by the period.
The upper is lower plus 100 ms without a clamp to A plus M
(`0x212D39–0x212D58`, `0x212DDD–0x212E1F`).

The same calculation with projected clock yields a projected lower.
When that differs from saved lower and is strictly below A plus M,
snapshot signed combo `+0x70` into `+0x1314` and clear struck `+0x11AC`
and deferred `+0x11B4`. This emits no per-ID feedback and leaves the
hit latch `+0x13B8` as stored. The saved bounds still describe the
earlier clock's pulse. The clear sets a scratch condition that can also
be set independently by projected clock above the saved upper. Its
alternate candidate continuation requires action 32 or 33
(`0x212E25–0x212EF2`, `0x214A17–0x214A26`).

The same active action-11 pulse path then draws at `0x212F7C` if earlier
clock minus the frame step is below A, or if the signed truncated pulse
quotients for earlier and projected time differ. This appends sound 15
at pitch one plus draw times 0.25 divided by 32,767. It occurs after
any pulse clear and before ordinary deferred processing. The second
pulse-sound branch requires action 91 (`0x212EF8–0x212FD2`).

All thirteen actions then share two deferred passes. At clock at or below
A, snapshot combo into `+0x1314`, give feedback for eligible deferred IDs,
clear both sets and zero `+0x13B8`. At clock at or above saved upper, give
the same feedback and clear deferred alone. Each pass visits signed
64-bit deferred IDs in order, skipping IDs in struck or with no live
map resolution. It calls `0x03CF20` first and then appends a zero-amount
type-4 hit addressed from the attacker to the resolved target at its live
position. For target nonzero roll, class 4 and specialization 1, the
helper adds binary32 0.25 to target mana with cap one, installs resident
type 11 with 30,000 ms duration, zero strength and zero resident source,
and appends a self-addressed buff event before the type-4 hit. These
passes use no draw (`0x213058–0x213200`, `0x2132CC–0x213448`,
`0x03CF20–0x03CFE1`).

After these passes, a clear `+0x13B8`, projected clock at least A plus M,
live clock below total duration and zero attacker roll admit a combo
reset between two speed samples. The native clock becomes the signed
truncation of the binary32 product of the live clock converted to
binary32 and the binary32 ratio of old speed to new speed. Resource
preparation follows. Admission to the candidate pass uses the resulting
clock strictly above the saved lower and at or below the saved upper.
Upper equality can both expire prior deferred IDs and admit candidates
(`0x213448–0x2134EE`, `0x2137F7–0x213808`).

## 3. Resource, charge indicator and buff consumption

At live clock zero the body clears forced-critical byte `+0x13C0` and
recorded binary32 resource `+0x13B4`. The `0x044AE0` nominal cost lookup
subtracts 3, bounds the unsigned index through 92 and otherwise goes to
`0x044B75`. The bytes at `0x044B96–0x044BA2` for actions 9 through 21
are all ordinal 4. The original pointer table at `0x044B7C` makes this an
in-range selection of the positive-zero return at `0x044B75`. There is
no positive nominal cost debit for this group
(`0x044AE3–0x044AF9`, `0x213510–0x213553`).

The zero-cost continuation at `0x21364B` gives two scoped branches.

- Actions 17 and 20 join at `0x21367D`. Copy all live mana `+0x170` into
  `+0x13B4` and zero mana. Seek the first resident type 11 with
  `0x043F10`, which tests type without duration. If present, the
  `0x046AF0` update replaces that entry with type 11, zero duration,
  zero strength and zero resident source. Append the replacement buff
  event with attacker ID as source and target, then set `+0x13C0` to
  one. If absent, the cleared forced-critical byte stands. There is no
  draw here (`0x21364E–0x213654`, `0x21367D–0x21374F`, `0x02C520`,
  `0x046B24–0x046B33`, `0x046B71–0x046B8A`).
- Action 21 branches at `0x21365E–0x213660` to `0x21366E`, setting
  `+0x13B4` to binary32 one with no mana debit in that branch.

Then at zero or nonzero clock, strictly positive pending binary32
resource `+0x144` overwrites the recorded value, debits the full pending
amount from mana and clears `+0x13B8`. Action 11 alone in scope halves
the recorded value after the full debit. Pending resource is cleared and
ordered negative mana is floored at zero. The saved charge indicator
is true for strictly positive resulting `+0x13B4` or for action 11.
Pending resource can replace the 17 or 20 startup snapshot without
undoing the already appended buff event or forced-critical flag
(`0x2135A2–0x213646`, `0x2137D9–0x2137F1`).

## 4. Candidate geometry and live hit arguments

All actors here use live fixed position `+0x10` and binary32 size triple
`+0x80`. The ordinary center is the live attacker position plus a
fixed-converted local forward offset rotated by the live yaw at `+0x30`.
The following base reaches and forward offsets multiply attacker size X.

| Actions | Base reach | Forward offset | Branch evidence |
| --- | --- | --- | --- |
| 9, 13, 14, 18, 19 | 2 | 1 | Action compares at `0x213879–0x2138AE` join `0x2138B0` |
| 10, 12, 15, 16, 17 | 1.5 | 1.5 | Default at `0x2138D1` |
| 20, 21 | 1.5 then 2 | 1.5 | `0x2138E1–0x2138EF` doubles only the base |
| 11 | 1.5 then 3 | 0 | `0x213AD8–0x213B08` restores the live center and triples the base |

The two multiplications in the last two rows round separately in
binary32. Yaw rotation and fixed conversion are shared at
`0x21380E–0x213A29`. Connected mode goes from `0x213B25` to `0x214084`,
skipping mode-zero effects and terrain work before iteration.

The live creature map at world `+0x04` is traversed in ascending signed
64-bit key order, high word signed and low word unsigned when the high
words match. The struck and deferred keys come from the resident's live
64-bit ID at `+0x08`. A candidate must be nonnull, have positive binary32
health and be absent from the attacker's struck set. The horizontal and
vertical tests are strict. Horizontal uses base reach plus target size X
times 0.7 and the fixed squared XY distance to the attack center.
Vertical uses the absolute Z displacement in blocks, bounded by target
size Z times 0.5 plus attacker size Z plus base reach. Sight uses both
unshifted live positions with limit 200 blocks and enabled world objects.
The shared `0x1988A0` hostility test rejects identical pointers and uses
the live actor kinds, kind-1 classifier and status mask. Its early test
does not exclude the world-local pointer. A hostile target with nonzero
32-bit roll inserts its ID into deferred and advances without a draw,
hit, or struck insertion. Other admission rejects insert neither ID and
use no candidate draw (`0x214178–0x214441`, `0x214BB7–0x214BEB`).
The fixed-distance, ray and relationship details are in §§2–3 of the
[connected action-1 candidate contract](alpha-connected-melee-candidates.md).

For a hostile target with zero roll, form the unshifted live target
minus attacker vector in binary32 blocks. Set Z to zero and normalize
through the rounded reciprocal of length if squared length exceeds
binary32 0.01. Set Z to 0.25, then select critical as described in §5.
Scale the vector by the rounded square root of the binary32 attacker
size X divided by target size X, then by the action impulse
(`0x214441–0x214529`).

The impulse helper `0x044860` returns positive zero at `0x0449FA` for
attacker control byte `+0x60` equal to 3 or 5. Other kinds use the action
minus one as an unsigned index through 87. Out of range, the selector
returns 2 divided by binary32 speed at `0x0449D9`. The original byte
and pointer tables are `0x044A38` and `0x044A00`. For this group their
selections are as follows, using binary32 `0x047700` speed for division.

| Actions | Byte ordinal | Selected RVA | Impulse scalar |
| --- | --- | --- | --- |
| 9 | 0 | `0x044908` | 6 divided by speed |
| 10, 12, 16 | 13 | `0x0449D9` | 2 divided by speed |
| 11 | 4 | `0x0448D3` | 10 |
| 13, 14, 15 | 5 | `0x0448B2` | 6 divided by speed |
| 17, 20 | 6 | `0x044929` | 6 |
| 18, 19 | 3 | `0x044891` | 3 divided by speed |
| 21 | 7 | `0x044933` | Skill result times 30, plus 5 |

For action 21 the impulse site calls `0x03ED60` with action 21 and
auxiliary minus one. Its skill selector `0x03C9C0` subtracts 21, bounds
the unsigned index through 82 and otherwise returns minus one at
`0x03C9F1`. Index zero at `0x03CA08` holds byte zero, whose pointer at
`0x03C9F8` leads to integer skill index 6 at `0x03C9DC`. The negative
skill-selector fallback would return binary32 one at `0x03ED77`.
The actual action-21 call reads signed skill rank at `+0x1150`. If zero
and control kind nonzero, use signed truncated half level at `+0x190`
plus zero-extended byte `+0x1A8` plus one. The skill result is one minus
one divided by the sum of one and rank times 0.1. Conversions and each
floating operation round to binary32, including the subsequent times
30 and plus 5 (`0x03ED60–0x03EDDB`, `0x044933–0x04495D`).

The equipment collector `0x03DF60` first takes a qualifying category-3
subtype 6 or 7 from primary slot `+0xAA8` alone, then checks the same
priority for secondary `+0x990`. If neither qualifies, it subtracts 2
from the live action and bounds the unsigned index through 94. The
bounds fallback is `0x03E09F`. Original byte table `0x03E130` and
pointer table `0x03E120` give the following in-range selections.

| Actions | Byte ordinal and RVA | Weapon list in order |
| --- | --- | --- |
| 10, 18 | 0 at `0x03E061` | Secondary alone if category 3 |
| 11, 12, 16, 17, 20 | 1 at `0x03E07B` | Secondary then primary, each if category 3 |
| 9, 13, 14, 15, 19, 21 | 3 at `0x03E09F` | Primary if category 3, then secondary if category 3 subtype 12 |

This collector supplies the attack stat walk at `0x03DD21` and the
enhancement and first-weapon sound sites at `0x197624`, `0x1976F6` and
`0x197CE4`. Its category and subtype gates are at
`0x03DFA9–0x03E045` and `0x03E061–0x03E0CE`.

## 5. Ordered candidate draws, direct effects and success

On the zero-roll hostile path, critical is immediately true if
`+0x13C0` is nonzero. Otherwise the draw at `0x0444E5` tests whether
critical probability strictly exceeds draw divided by 32,767. This
includes the resident type-11 probability-one branch. Without type 11,
the probability is the `0x03EA40` result plus meter `+0x1190` times
binary32 0.15 (`0x2144B4–0x2144D6`, `0x0444A0–0x04451E`).

All actions in scope take the ordinary physical amount branch at
`0x21452E–0x214561`. The baseline is one except action 11 selects
positive zero through `0x21409B–0x2140A6`. The `0x03EC90` coefficient
lookup subtracts 5, bounds the unsigned index through 67 and sends
out-of-range indices to the exact-one return at `0x03ECE8`. Byte table
`0x03ED10` and pointer table `0x03ECF0` give this same target through
ordinal 7 for actions 9–19 and ordinal 1 for action 20. Action 21 is
index 16, byte 2, target `0x03ECDE`, reading binary32 0.1 from
`0x2FCB7C` (bits `0x3DCCCCCD`).

Multiply the selected coefficient by `0x03DC60`'s physical attack
result. Separately square recorded resource `+0x13B4`, multiply the
square by 5 and add the baseline. Multiply those two results. Then
draw at `0x2145C7` for variation 1.25 minus draw times 0.5 divided by
32,767, with a positive-zero addition after that subtraction. Multiply
variation by the prior amount and double the result if critical. Every
stage rounds in binary32. Plain uncharged action 11 still follows
both draw sites under their usual critical and variation gates even
with amount zero (`0x214561–0x214600`, `0x21467B–0x21469C`).

Before calling the hit helper, remember whether struck is empty and
the attacker signed stun `+0x12C` is at most zero. Insert target ID
and set `+0x13B8` to one. The call at `0x21476B` passes live target
and attacker, physical amount, critical, saved charge indicator, live
recorded resource, unshifted attacker position, impulse and the
current output collector and scratch argument. The actions-30-through-33
flag is false. The action argument is that attacker's unsigned live
action byte, the next argument is zero and the final guard-enabled
argument is one (`0x21469C–0x21476B`).

For each invocation, the following order continues after the caller
draws and insertion. It uses the shared mechanics in §§5–7 of the
[connected action-1 candidate contract](alpha-connected-melee-candidates.md).

| Branch in order | Draw, resulting output and continuation |
| --- | --- |
| Target health at most zero, or target equals world local creature | False before helper draws or appends (`0x196D7F–0x196DAE`). |
| Target control kind 1 with status `+0x124` mask `0x80` or contact `+0x5C` mask `0x20` | Draw `0x196EF9`, sound 24 then type-3 hit, false. Connected record application is bypassed (`0x196EC1–0x197016`). |
| Guard classifier `0x044590` true and target guard `+0x174` positive | Using the prior guard, raise target mana with cap one and pending resource capped to the resulting mana, then reduce guard by caller amount divided by `0x03E190` result with floor minus one. Append type-1 hit, then draw `0x1971C0` and append sound 24. Return false (`0x19701B–0x197230`). |
| Positive effective resident type-6 ward | Append type-5 hit, then draw `0x197478` and append sound 91. A remaining amount at or below zero returns true here. Otherwise continue physical defense. Connected record application is bypassed (`0x197235–0x19750B`). |
| Continuing ordinary hit | Defense and ordered equipment enhancements, including any immediate enhancement self-heal and its self record, precede the ordinary sound draw `0x197D1B`. The enabled flag writes binary32 one to target `+0x1184` before that sound (`0x19750B–0x197AF3`). |
| Critical ordinary hit and target kind other than 6 | Draw `0x198258` for one plus draw times 0.1 divided by 32,767. Append the selected species sound at the live target position if the native species table produces one. An unlisted species still draws (`0x1981CB–0x1983EC`). |
| Continuing ordinary hit with target signed stun below minus 3,000 and target kind other than 6 | Draw `0x198545` after any species draw. When probability strictly exceeds the normalized draw, append fixed-pitch sound 23 and supply the outgoing hit's stun fields (`0x1983EC–0x1986D5`). |

The ordinary sound at the target uses volume one. Base pitch is binary32
0.9 plus draw times 0.2 divided by 32,767. The original critical and
noncritical tables at `0x198728/0x19870C` and `0x198758/0x19873C`
index the first collected subtype directly with unsigned bound 17.
Subtype 0 or 1 adds binary32 0.1 to pitch. Subtypes 0–1 and 15–16 use
ordinary noncritical/critical sound pair 1/2. Subtypes 3–4 use 7/8,
subtype 5 uses 3/4 and subtypes 6–8 use 9/10. The rest, including no
weapon, use 5/6.
For live action 10 the branch at `0x197DFE–0x197E0C` overrides the
ordinary ID with 5 after the subtype pitch adjustment, in both critical
states. The same `0x197D1B` draw supplies that pitch.

After the ordinary sound, each matching resident type-1 entry on the
target multiplies the current amount by its strength in list order
(`0x198023–0x19804B`). Passed action argument 17 then appends a
resident type-4 buff to the target, with strength equal to the resulting
hit amount times binary32 0.1, duration 3,000 ms and resident source ID
equal to the attacker ID. It appends a separate buff event with the
same descriptor, attacker source ID and target ID. The append is to
the resident list tail and does not replace an existing entry. It is
deterministic and precedes both the optional critical species sound
and the stun draw. Only a continuing ordinary hit reaches this site
(`0x19804D`, `0x1980BF–0x1981C9`).

In the stun path, live attacker action 12 selects a binary32 seed of one
at `0x198419–0x198429` before classifier scaling. Action 16 uses the
passed recorded resource like the other scoped actions. The common
rule multiplies the seed by 0.15 when target `0x0445D0` is true and by
0.1 when attacker `0x0445D0` is false and attacker kind is nonzero.
Target live action 84 then sets the seed to one. The native level scaling
and cap run before multiplication by 0.9. Each resident type-1 target
entry suppresses that chance to zero, and the admitted path still draws
(`0x1983EC–0x198545`). The shared stun level calculation is in §7 of the
[connected action-1 candidate contract](alpha-connected-melee-candidates.md).

The continuing helper appends the ordinary hit and returns true.
Connected mode bypasses target application at `0x1986B8–0x1986D5`.
The enhancement self-heal writes attacker health directly when its
shared prerequisites hold (`0x19798D–0x197A71`). The alternate draw
`0x196E20` needs attacker action 28, and `0x197E3D` needs a null
attacker. The scoped call supplies a live attacker throughout.

False leaves the prior struck insertion and latch set. True, including
a fully absorbed ward, gives one combo increment if the pre-insertion
empty-struck and nonpositive-stun condition was true. It visits the
`+0x139C` tree through `0x0453B0` without a draw or mutation, then
clears combo timer `+0x74`. Independently of combo, true with the
saved charge indicator and attacker class 4, specialization 0 adds the
live recorded resource to binary32 `+0x1190`, capped at one. This happens
before the next candidate (`0x214772`, `0x214899–0x214906`).

World mode and local-pointer checks at `0x214906–0x21491B` bypass the
success reward draws `0x214949` and `0x2149B4`. The success byte is set
at `0x214A05`. The alternate candidate branch requires action 32 or 33.
After the last candidate, the scope reaches the common continuation
with only an optional deterministic duration read after success and
the equipment gate. It also bypasses the ordinary combo timeout
increment for this attacker (`0x214A17–0x214A26`, `0x214BF1–0x214C2D`,
`0x215BA8–0x215BAE`, `0x211F41–0x211F56`).

## 6. Following buff pass and limits

After this action body, the actor's resident-buff pass subtracts the
signed frame step from each duration and removes entries with remaining
duration at or below zero. Thus the zero-duration type-11 replacement
is available during the candidate walk and then expires in the
attacker's own pass. A surviving type-4 buff reaches the type-4 handler
during its holder's visit. Nonzero world `+0xB4` bypasses the periodic
type-4 hit path there (`0x2120E8–0x212163`, `0x218310–0x218330`,
`0x2183FA–0x2184C2`). The later short-clock proximity walk at
`0x219E60–0x219F02` also bypasses connected nonlocal attackers at
`0x219EE5–0x219EF4`.

A later action-zero test at `0x218597` enters the deferred feedback
and clears both sets at `0x218701–0x218708`. Any still-live action in
this group bypasses that idle cleanup.

The draw order here covers the common action body, declared helper
branches and immediate buff boundary. The general actor turn, input
producers, movement, packet receipt and application, dedicated authority
and standalone consequences remain outside scope. The native speed,
equipment stat, relationship classifier and ray intersection kernels
are shared dependencies. Runtime acceptance, nonfinite state and the
zero-period action-11 case remain open.
