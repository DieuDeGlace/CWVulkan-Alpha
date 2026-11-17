# Alpha connected melee actions 2 through 7

**FINAL / STABLE / HELD**

Supported image is the original 32-bit Alpha `Cube.exe`, 3,885,568 bytes,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs at image base `0x00400000`. Action and sound numbers are
decimal. The domain is finite native creature state with nonnegative elapsed
milliseconds, representable intermediate arithmetic and positive divisors.
For action 5 the integer pulse period must be positive.

This contract covers a living attacker during the world turn at `0x20C510`
with world byte `+0xB4` nonzero and actor pointer different from world
`+0xB8`. It begins at the common action dispatch and ends at its bookkeeping
and cleanup. [Alpha connected action-1 melee candidates](alpha-connected-melee-candidates.md)
gives the shared ray, relationship, hit and equipment rules referenced below.

## 1. Dispatch, durations and sound before the window

The action byte `+0x68` uses the dispatch at `0x211E04–0x211E1B`. Actions
1 through 7 all select `0x212357` through tables `0x222CCC` and `0x222C84`.
Connected mode takes the branch at `0x212364` directly to preparation.
For an action-5 attacker with class byte `+0x140` equal to 4,
specialization byte `+0x141` equal to 1 and live signed clock `+0x6C`
equal to zero, `0x21239F` writes 600 to the 32-bit roll field `+0x128`.

The anticipation and middle readers `0x03CAA0` and `0x047310` take the live
action. Each converts a binary32 numerator divided by the rounded binary32
product of `0x047700`'s speed result and actor binary32 `+0x17C` into signed
milliseconds by truncating toward zero. The total at `0x03D1A0` separately
reads speed for the anticipation, middle and recovery legs and adds their
truncated results. These are the selected numerators.

| Action | Anticipation at `0x03CAA0` | Middle at `0x047310` | Recovery leg at `0x03D1A0` |
| --- | --- | --- | --- |
| 2 | 100 at `0x03CC50` | 200 at `0x0473BD` | 500 at `0x03D994` |
| 3 and 4 | 100 at `0x03CC50` | 100 at `0x047353` | 300 at `0x03D8AF` |
| 5 | 100 at `0x03CC84` | 400 at `0x0473F1` | 300 at `0x03D8AF` |
| 6 | 50 at `0x03CD06` | 150 at `0x047387` | 500 at `0x03D994` |
| 7 | 50 at `0x03CD06` | 150 at `0x047387` | 200 at `0x03D8CB` |

The selector tables are `0x03CE24/0x03CEB0`, `0x0475A8/0x047628` and
`0x03D9C8/0x03DA54`, `0x03DAC0/0x03DB40`, `0x03DBAC/0x03DBEC`.
The main turn reads these durations multiple times. In the descriptions
below, anticipation and middle denote their stable results. The earlier
clock sample is the live signed clock before this body's window adjustment.
Projected clock adds the signed frame step to that sample.

The following action-body draws precede bound preparation and candidate
iteration. Both sounds use volume one and the attacker's live position
converted to binary32 blocks. Each pitch operation rounds in binary32.

1. At clock zero with the `0x044230` weapon predicate true, draw at
   `0x212442` and append sound 17, with pitch one plus draw times 0.1
   divided by 32,767. The predicate requires category 3 at `+0xAA8` and
   subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18 at `+0xAA9`.
   This is the route at `0x2123F3–0x212485`.
2. If earlier clock is at or below anticipation and projected clock is
   strictly above it, draw at `0x212738` and append sound 15. Form pitch
   as 0.9 plus draw times 0.2 divided by 32,767, multiply by 1.5, then
   multiply by 0.5 when the same weapon predicate is true. The start
   sound can precede this swing in the same turn
   (`0x2126F3–0x212712`, `0x212738–0x212762`,
   `0x2129D7–0x212A45`).

## 2. Saved bounds, pulse cleanup and expiry

For actions 2, 3, 4, 6 and 7 the ordinary bounds at
`0x212B43–0x212C05` are retained. The lower is anticipation plus half
middle minus 100, raised to anticipation when smaller. The upper is
anticipation plus half middle, capped at anticipation plus middle minus
one. Half middle uses signed truncation toward zero. The selectors at
`0x212C0B–0x212CDE` leave these actions on the ordinary continuation.

Action 5 branches at `0x212CD8–0x212CDA` and replaces those bounds. If the
earlier clock is below anticipation or at least anticipation plus middle,
both bounds are the anticipation read at `0x213041`. Otherwise the signed
pulse period is middle divided by three, truncating toward zero
(`0x212D60–0x212D83`). In that active interval the lower is anticipation
plus period times the truncated quotient of elapsed time since anticipation
divided by period. Upper is lower plus 100 ms. The upper is stored without
a clamp to anticipation plus middle (`0x212DDD–0x212E1F`).

The pulse path also computes a projected lower by the same division using
projected clock. If it differs from the saved lower and is strictly below
anticipation plus middle, copy combo count `+0x70` to `+0x1314` and clear
both the struck set `+0x11AC` and deferred set `+0x11B4` immediately.
This clear has no per-ID feedback and leaves `+0x13B8` as stored. It
happens before the ordinary deferred passes. A projected clock strictly
above the saved upper also marks the scratch pulse condition. Its tested
continuation at `0x214A1A–0x214A26` requires action 32 or 33, so it adds
no alternate candidate route for action 5. The active action-5 pulse
branches directly to deferred preparation at `0x212EFC`, past the
pulse-sound path. Evidence is `0x212E25–0x212EFC`.

All six actions then share the two deferred passes. At live clock at or
below anticipation, snapshot combo into `+0x1314`, process earlier deferred
IDs, clear both sets and zero `+0x13B8`. At live clock at or above the saved
upper, process earlier deferred IDs and clear the deferred set. These use
the feedback rule in section 1 of the action-1 contract, including the
same target benefit at `0x03CF20` and type-4 append without a draw.
The earlier pulse clear can leave both passes with empty sets
(`0x213058–0x213200`, `0x2132CC–0x213448`).

After those passes, with `+0x13B8` clear, projected clock at least
anticipation plus middle, live clock below total duration and attacker roll
zero, the body clears combo between two `0x047700` reads. It multiplies
the live clock converted to binary32 by the binary32 first speed divided
by the second, and truncates to the signed clock. The saved bounds survive
this change. Resource preparation follows. The target pass requires the
resulting live clock strictly above the saved lower and at or below the
saved upper. Upper equality can therefore expire earlier deferred IDs
and still admit the current target pass (`0x213448–0x2134EE`,
`0x2137F7–0x213808`). Later action-zero cleanup at `0x218597–0x218713`
uses the shared deferred rule when its own action test is met.

## 3. Resource and buff preparation

At live clock zero `0x213520/0x213527` clear forced-critical byte `+0x13C0`
and recorded binary32 resource `+0x13B4`. The `0x044AE0` cost selector at
`0x044AF2–0x044AF9` routes actions 2 through 7 to zero at `0x044B75`.

Action 5 then copies all live mana `+0x170` into `+0x13B4` and sets mana
to zero. It seeks the first resident buff type 11 through `0x043F10`,
which tests the type byte without checking duration. If present, the
`0x046AF0` update replaces that resident entry with a type-11 record of
zero duration, zero strength and zero resident source ID. A buff event
carrying the new record and the attacker's own ID as both source and
target is appended, then `+0x13C0` becomes one. The replacement event
precedes all candidate activity and uses no draw. Evidence is
`0x213656–0x213658`, `0x21367D–0x21374F`, `0x02C520`,
`0x046B24–0x046B33` and `0x046B71–0x046B8A`.

With any of these actions, a strictly positive pending binary32 resource at
`+0x144` overwrites the recorded value, subtracts the full pending value from
mana and clears `+0x13B8`. Action 5 alone in this set then halves the recorded
value. Pending resource is zeroed and mana floored at zero. The indicator
saved for the hit is whether the resulting recorded value is strictly
positive. The pending path runs at zero and nonzero live clock. In action
5 a pending value can therefore replace the startup mana snapshot without
undoing the already emitted buff event or forced-critical byte
(`0x2135A2–0x213646`, `0x2137D9–0x2137F1`).

## 4. Geometry, candidates and per-action hit inputs

Actions 2, 3, 4, 6 and 7 take `0x2138B0` through their respective compares
at `0x213883`, `0x213897`, `0x213892`, `0x2138A1` and `0x21389C`.
They use twice attacker size X as base reach and one attacker size X as
the forward center offset, the same geometry as action 1. Action 5 follows
`0x2138D1` and reaches `0x2139C5` with binary32 1.5 times size X for both
base reach and forward offset. All six yaw-rotate that local offset,
convert it to fixed point and add it to the attacker's live position
(`0x21380E–0x2138E1`, `0x2139C5–0x213A29`).

The connected branch at `0x213B1E–0x213B25` skips the mode-zero output and
terrain span and selects amount base one at `0x214084–0x2140AE`. The
creature map iteration at `0x214178` and admission through `0x214441`
are shared. The order is ascending signed 64-bit world map key. For each
resident pointer the candidate ID used in struck and deferred sets is
its live 64-bit actor ID. The ordered tests are nonnull pointer, positive
health, absent from struck, strictly inside horizontal and vertical range,
sight, hostility and zero target roll. A hostile rolling target inserts
its ID in the deferred set and advances without a draw or immediate
output. The geometry uses the shared fixed distance and vertical tests
from section 2 of the action-1 contract with the base and center above.
Sight uses unshifted live positions, limit 200 and enabled world objects
(`0x214223–0x21425A`, `0x214265–0x21427A`, `0x2143CD–0x2143F1`).

The common impulse at `0x214441–0x214529` uses the unshifted live position
difference. It sets Z to zero and normalizes when the squared length
exceeds binary32 0.01. It then assigns Z to 0.25 and scales by the rounded
square root of attacker size X divided by target size X. It then scales
by the factor from `0x044860`, using the live action. That reader first
returns zero for attacker controller kind 3 or 5. Other roles use the
factors below, with binary32 division by the `0x047700` result where shown.

| Action | Impulse factor and selected RVA | Ordinary weapon list fallback |
| --- | --- | --- |
| 2 | 3 divided by speed at `0x0448E7` | Category-3 item at `+0x990` alone |
| 3 | 3 divided by speed at `0x0448E7` | Primary then secondary as below |
| 4 | Constant 4 at `0x04498D` | Category-3 item at `+0x990` alone |
| 5 and 6 | 3 divided by speed at `0x044891` | Primary then secondary as below |
| 7 | 2 divided by speed at `0x0449D9` | Category-3 item at `+0x990` alone |

The impulse dispatch is `0x044876–0x04488A` with tables
`0x044A00/0x044A38`. For weapons, the priority checks at
`0x03DFA9–0x03E045` first take a category-3 subtype 6 or 7 at `+0xAA8`
alone, then the same qualifying `+0x990` alone. Otherwise the live-action
switch at `0x03E047–0x03E05A` selects `0x03E061` for actions 2, 4 and 7
and `0x03E09F` for actions 3, 5 and 6. The latter path collects
`+0xAA8` if category 3 and then `+0x990` if category 3 subtype 12.
A slot failing its predicate contributes no weapon. The tables are
`0x03E120/0x03E130`. This shared helper supplies the attack stat equipment
walk at `0x03DD21`, and the enhancement and first-weapon sound choices at
`0x197624`, `0x1976F6` and `0x197CE4`.

For every zero-roll hostile target, `0x2144B4–0x2144D6` uses the critical
flag immediately when `+0x13C0` is nonzero. Otherwise `0x0444A0` performs
the shared critical draw at `0x0444E5`, even when a resident type-11 buff
sets probability to one. The amount branch at `0x21452E–0x214561`
selects the ordinary physical attack path for all six actions. The live
action in `0x03EC90` selects amount factor one in every case, through
`0x03ED10/0x03ECF0` to `0x03ECE8`. Multiply this by the binary32
`0x03DC60` attack result and by one plus five times squared recorded
resource. Then use the draw at `0x2145C7` for the factor 1.25 minus
draw times 0.5 divided by 32,767, with the same positive-zero addition
and binary32 stages as action 1. Critical doubles the final amount
(`0x214561–0x214600`, `0x21467B–0x21469C`).

Before the hit call, remember whether struck was empty and the attacker's
signed stun `+0x12C` was at most zero. Insert this candidate's ID in struck
and set `+0x13B8` to one. At `0x21476B` call `0x196D30` with live
attacker and target, caller amount, critical, saved resource indicator,
live recorded resource, unshifted attacker position, impulse and current
output collector. The action argument is the unsigned live action byte,
so the values passed here are 2, 3, 4, 5, 6 and 7 respectively. The
actions-30-through-33 flag is false, the argument after the action is zero,
and guard enabled is one (`0x21469C–0x21476B`).

## 5. Hit result and ordered draws

The actions share the following ordered hit-helper paths. The caller's
optional critical and mandatory amount draw have already happened if the
helper is called. The original uses the target position, defense, protection,
enhancement, append and stun rules in sections 5 through 7 of the action-1
contract, using the current weapon list above.

| Hit-helper branch in order | Draws and result |
| --- | --- |
| Target health retest fails or target equals world local creature in connected mode | No helper draw or append. False at `0x197014` (`0x196D7F–0x196DAE`). The earlier caller writes remain. |
| Kind-1 protection admits contact or status mask | Draw at `0x196EF9`, append sound 24 before type-3 hit, false. The mode gate skips application (`0x196EC1–0x197016`). |
| Guard is active and target guard value is positive | Mutate live guard, mana and pending resource. Append type-1 hit before sound 24, with draw at `0x1971C0`, false (`0x19701B–0x197230`). |
| Effective resident type-6 ward is positive | Append type-5 hit before sound 91, with draw at `0x197478`. Stop with true if the remaining amount is at most zero, otherwise continue physical defense with that amount. Connected mode skips application of the ward record (`0x197235–0x19750B`). |
| Continuing ordinary hit | Physical defense, ordered equipment enhancements and any self-heal precede the target sound draw at `0x197D1B`. The current weapon list supplies the first weapon, or the empty-list sound choice. Target resident type 1 multiplies the record amount for each matching entry. The hit-helper flag writes one to target `+0x1184` before that ordinary sound. |
| Critical ordinary hit and target controller kind differs from 6 | Draw at `0x198258` after the ordinary sound. The native species table governs the extra sound append. An unlisted species still draws (`0x1981CB–0x1983EC`). |
| Ordinary hit and target signed stun strictly below minus 3,000, controller kind different from 6 | With these gates met, draw at `0x198545` after any critical species draw. The passed recorded resource drives the probability, subject to the classifier, level, target action 84 and type-1 buff rules. A passing roll emits fixed-pitch sound 23 before the ordinary hit append and supplies its stun fields (`0x1983EC–0x1986D5`). |

The special live-action selectors at `0x196DBF–0x196DD3`,
`0x197AC8`, `0x197D0B`, `0x197DED–0x197DFE` and
`0x198412–0x19847F`, and the action-argument selectors at
`0x197C5A` and `0x19804D`, leave actions 2 through 7 on the ordinary
continuations above. The false actions-30-through-33 flag selects physical
defense at `0x19750B–0x19751A` and ordinary sound at `0x197B05–0x197B09`.
The final ordinary hit appends before the connected success return.
Connected mode skips mode-zero target application at
`0x1986B8–0x1986D5`. The enhancement self-heal remains an immediate
attacker health write at `0x19798D–0x197A71` when its prerequisites
from the action-1 contract hold.

## 6. Success bookkeeping and limits

False leaves the struck insertion and latch in place and advances. True
can come from a fully absorbed ward. On true, the pre-insertion empty
struck and nonpositive attacker stun condition gives one combo increment,
then the ordered tree walk at `0x0453B0` and then combo timer `+0x74`
cleared. The walk visits the `+0x139C` tree, with no field write, append
or draw. A false result on an earlier key can thus consume the empty-set
condition for a later success. Independently of combo, true with saved
resource indicator and attacker class 4 specialization 0 adds the live
recorded binary32 resource to meter `+0x1190` capped at one. This is done
before the next candidate (`0x214772`, `0x214899–0x214906`).

The mode and local-pointer comparison at `0x214906–0x21491B` skips
success reward draws `0x214949` and `0x2149B4` for the connected nonlocal
attacker. The success byte is set at `0x214A05`. The cases at
`0x214A17–0x214A26` exclude all six actions from the alternate hit branch.
After the last candidate the continuation at `0x214BF1–0x214C2D` goes
to common bookkeeping, with an optional deterministic duration read and
no further candidate draw. The connected nonlocal actor also skips the
ordinary combo timeout increment at `0x211F41–0x211F56`.

The stock speed, equipment stat and ray intersection kernels are
dependencies. The fixed outputs and random order here stop at the common
action and hit boundaries above. Other action bodies, standalone and
dedicated authority, and malformed or nonfinite state are outside scope.

## 7. Presentation counter read sites

Creature construction `0x03B941/0x03B94B/0x03B955` and reset
`0x046425–0x046439` zero the presentation accumulators. The presentation
creature walk loads and increments signed timer `+0x13D0` before comparing
it to 10,000. A value strictly greater clears the signed elapsed
counter `+0x13CC` and binary32 amount accumulator `+0x13C8`, then the
same visit adds the frame step to `+0x13CC`. The amount walk adds a
strictly positive hit amount to the resolved source's `+0x13C8` and
resets that source's `+0x13D0`, after resolving the target
(`0x0948F0–0x094937`, `0x0949B0–0x094A31`).

The original executable-text displacement census found those
read-modify-write and threshold reads. Other literal hits belonged
to stack slots or shared immediate encodings. No traced stock drawing or
independent decision reader for the two accumulators was found. An
untraced computed-base reader remains open.
