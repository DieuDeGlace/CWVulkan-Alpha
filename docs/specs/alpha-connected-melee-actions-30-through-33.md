# Alpha connected melee actions 30 through 33

**FINAL / STABLE / HELD**

The supported image is original 32-bit Alpha `binary/Cube.exe`, 3,885,568
bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs at image base `0x00400000`. Action IDs, sound IDs and
selector ordinals are decimal. Selector bytes, little-endian preferred
virtual-address pointers and numeric literals were read directly from the PE.

This covers a living attacker in the `0x20C510` world turn with mode byte
`+0xB4` nonzero and attacker pointer different from world local pointer
`+0xB8`. The domain is finite native state with a nonnegative signed
frame step, representable intermediates and positive stat divisors.
The preceding contract describes the shared timing and bookkeeping.
[Connected melee actions 9–21](alpha-connected-melee-actions-9-through-21.md)

## 1. Entry and selector evidence

The dispatch at `0x211E04–0x211E1B` zero-extends actor byte `+0x68`,
subtracts one and bounds the unsigned index through 109. It uses
byte table `0x222CCC` and pointer table `0x222C84`. The bounds
fallback is `0x215EC0`.

| Live action | Index | Byte ordinal | Selected RVA | Connected nonlocal entry result |
| --- | --- | --- | --- | --- |
| 22, 23, 26 | 21, 22, 25 | 2 | `0x21707A` | Goes to `0x211F41` via mode and local comparison at `0x217087–0x217095` |
| 24, 25, 27 | 23, 24, 26 | 3 | `0x215BB3` | Goes to `0x215EC0` via `0x215BC0–0x215BCE` |
| 28 | 27 | 4 | `0x215C68` | Goes to `0x215EC0` via `0x215C75–0x215C83` |
| 29 | 28 | 17 | `0x215EC0` | In-range entry of the fallback pointer |
| 30, 31, 32, 33 | 29, 30, 31, 32 | 0 | `0x212357` | Nonzero world mode continues to `0x212378` via `0x212364` |

Only 30 through 33 enter this common melee body from the scoped dispatch.
The roll store at `0x21239F` requires action 5, 17 or 20 and is bypassed.

For this group the anticipation and recovery legs each divide the
selected numerator by the binary32 product of speed from `0x047700` and
actor factor `+0x17C`, rounding the division to binary32 and truncating
to signed milliseconds. The middle leg is a fixed integer. The total
reader at `0x03D1A0` separately selects and adds the three integer
results. Anticipation and middle use live action when supplied minus one.

| Actions | Anticipation A | Middle M | Recovery | A, M, recovery ordinals |
| --- | --- | --- | --- | --- |
| 30, 32 | Scaled numerator 800 | Fixed 600 ms | Scaled numerator 100 | 15, 13, 6 |
| 31, 33 | Scaled numerator 1,600 | Fixed 1,200 ms | Scaled numerator 100 | 16, 14, 6 |

The timing selectors use unsigned action directly, with zero subtracted.
The shared ordinals for each pair have the following original targets.

| Leg | Unsigned maximum | Byte and pointer tables | Selected targets in ordinal order | Bounds fallback |
| --- | --- | --- | --- | --- |
| A at `0x03CAA0` | 105 | `0x03CEB0`, `0x03CE24` | `0x03CBD6`, `0x03CBF0` | `0x03CDF8` |
| M at `0x047310` | 104 | `0x047628`, `0x0475A8` | `0x04743F`, `0x047507` | `0x04757C` |
| Total A | 105 | `0x03DA54`, `0x03D9C8` | `0x03D2D2`, `0x03D2EC` | `0x03D4EB` |
| Total M | 104 | `0x03DB40`, `0x03DAC0` | `0x03D644`, `0x03D718` | `0x03D7DD` |
| Total recovery | 104 | `0x03DBEC`, `0x03DBAC` | `0x03D949` | `0x03D994` |

## 2. Sound and effect output before iteration

Earlier clock is the live signed value in `+0x6C` before retiming.
Projected clock adds the signed frame step. Sound positions in this
section are the attacker's live position converted to binary32 blocks.
The action compares at `0x2123CF–0x2123ED` enter `0x212495` and bypass
the ordinary clock-zero weapon sound test.

1. If earlier clock is strictly below A and its signed truncated
   quotient by 200 differs from projected clock's quotient by 200, draw
   at `0x212502` and append sound 36. Pitch is one plus draw times 0.25
   divided by 32,767. Volume is one. The quotients use absolute action
   clocks, and a change across several buckets still gives one draw
   and one append (`0x212495–0x21254C`).
2. If earlier clock is strictly below A and projected clock is at
   least A, append sound 41 for specialization byte `+0x141` equal
   to one, otherwise sound 38, both at pitch and volume one. Then
   append an effect to the collector's separate `+0x10` list. This
   crossing uses no draw (`0x212552–0x2126DC`).
3. Earlier clock at or below A and projected clock strictly above A
   draws at `0x212738` and appends ordinary swing sound 15. Pitch
   starts at 0.9 plus draw times 0.2 divided by 32,767, then is
   multiplied by 1.5 and also by 0.5 if the shared weapon predicate
   `0x044230` passes. Volume is one
   (`0x21248A–0x212490`, `0x2126F3–0x212762`, `0x2129D7–0x212A45`).
4. Inside the active pulse path in §3, draw at `0x212F7C` if earlier
   clock minus the frame step is below A or if the current and
   projected pulse quotients differ. Append sound 15 at volume one
   and pitch one plus draw times 0.25 divided by 32,767. This occurs
   after any early pulse reset and before the deferred passes
   (`0x212EF8–0x212FD2`).

All pitch stages use binary32. The crossing in step 2 includes projected
equality to A. The swing in step 3 requires projected clock above A.
The active pulse in step 4 can start with earlier clock equal to A.

The step-2 effect starts at live fixed position `+0x10` plus the
binary32 triplet `+0x160` converted to fixed point. Each conversion is
a binary32 product with 65,536 followed by truncation to signed
64-bit. The record stores that fixed triple at offset `+0x00`, a
binary32 triple of 0, 0, 2 at `+0x18`, binary32 0.25 at `+0x34`, a
32-bit value 20 at `+0x38`, and binary32 8 at `+0x40`. The 32-bit
selector at `+0x3C` is 2 for specialization one, with four explicit
binary32 components 0, 0.2, 1, 1 at `+0x24`. Other specializations
store selector 1 through the alternate branch. The field use in the
drawing consumer remains open (`0x21258E–0x2126DC`, `0x02C460`,
`0x0465ED0`, `0x086470`).

## 3. Saved pulse bounds and resource timing

All four actions replace the ordinary bounds via `0x212CC4–0x212CD2`.
If earlier clock is below A or at least A plus M, both bounds become A.
Inside that interval, actions 30 and 32 divide M by six, while actions
31 and 33 divide M by twelve. The results are both 100 ms. The clock
relative to A is divided by that period with signed truncation. The
saved lower is A plus period times the truncated quotient and upper
is lower plus 100 ms. The upper is saved without a clamp to A plus M
(`0x212CEC–0x212D21`, `0x212D85–0x212E1F`, `0x213037–0x213052`).

The same calculation with projected clock gives the projected lower.
When it differs from saved lower and is strictly below A plus M,
snapshot combo `+0x70` into `+0x1314` and clear struck `+0x11AC` and
deferred `+0x11B4` immediately. There is no per-ID feedback or
`+0x13B8` clear at that site. The saved bounds continue to describe the
earlier clock's pulse. This early reset sets a scratch flag initially
cleared at `0x212CB5`. Projected clock strictly above saved upper also
sets that flag independently. For a nonfinal pulse, projected clock
equal to saved upper already selects the next pulse lower and triggers
the reset. At final pulse upper equality, the projected lower is A
plus M, so neither flag condition passes. The flag is later used by the
32/33 branch (`0x212E25–0x212EF2`, `0x214A26`).

The common deferred passes follow the pulse presentation. At clock at
or below A, snapshot combo, visit eligible deferred IDs, clear both
sets and zero the hit latch `+0x13B8`. At or above saved upper, visit
eligible deferred IDs and clear deferred alone. Each pass walks the
signed 64-bit deferred IDs absent from struck and still resolving in
the live map. It runs `0x03CF20` then appends a zero-amount type-4 hit.
No draw is used. With latch clear, projected clock at least A plus M,
live clock below total duration and attacker roll zero, the common
retiming clears combo between two speed reads and multiplies live clock
by the binary32 old/new speed ratio before truncating to signed
milliseconds. The saved bounds are kept. The resource stage runs next,
then the resulting clock must be strictly above saved lower and at or
below saved upper for either candidate route
(`0x213058–0x213200`, `0x2132CC–0x2134EE`, `0x2137F7–0x213808`).
[Shared deferred feedback and retiming](alpha-connected-melee-actions-9-through-21.md#2-earlier-sounds-saved-timing-and-expiry)

Nominal cost at `0x044AE0` subtracts 3, bounds the unsigned index through
92, and uses bytes `0x044B90` and pointers `0x044B7C` with bounds
fallback `0x044B75`. Actions 30 and 32 use normalized indices 27 and
29, ordinal 1, reaching the positive-zero return at `0x044B75`.
Actions 31 and 33 use indices 28 and 30, ordinal 2, reaching
`0x044B0A`. That body returns positive zero on finding resident type 9
and otherwise binary32 0.3. It tests the buff type byte without a
duration filter (`0x044AE3–0x044AF9`, `0x044B0A–0x044B38`).

- Actions 30 and 32 clear forced-critical byte `+0x13C0` and
  recorded binary32 resource `+0x13B4` at live clock zero. Their
  nominal result is zero. Nonzero live clock bypasses these clears
  (`0x2134F7–0x213553`, `0x21364B–0x213668`).
- Actions 31 and 33 branch at `0x2134FA–0x213504` before that zero-clock
  initialization, including when clock is zero. Their separate
  crossing uses the current clock after any retiming and requires
  current clock at or below A and current clock plus step strictly
  above A. On it, write binary32 0.1 into `+0x13B4`, then debit the
  nominal cost from mana `+0x170` and floor ordered negative mana
  to zero. This leaves `+0x13C0` as stored
  (`0x213754–0x2137D4`).
- For all four, strictly positive pending binary32 `+0x144` then
  overwrites recorded resource, debits its full value from mana
  and clears `+0x13B8`. The finalization zeros pending resource,
  floors ordered negative mana at zero and saves whether resulting
  recorded resource is strictly positive. None of this group halves
  the pending value (`0x2135A2–0x213646`, `0x2137D9–0x2137F1`).

## 4. Candidate walk and hostile hit inputs

Actions 30 and 32 use base reach 3 blocks, and actions 31 and 33 use
5 blocks. These are constants in binary32. All four take the center
equal to live attacker fixed position plus the converted `+0x160`
triplet, using the conversion in §2. The sight start is that center
with an added positive Z equal to the fixed-converted base reach.
Geometry still uses the center before that positive Z addition.
Sight ends at live target position, with limit 200 and world objects
enabled (`0x213947–0x213979`, `0x213A32–0x213A75`,
`0x214265–0x2142D6`, `0x2143CD–0x2143F1`).

The connected jump at `0x213B25` bypasses the mode-zero output and
terrain span. Before iteration, action 31 alone tests current clock
at least A and seeks resident type 9 through `0x043F10`. If found,
replace its first resident instance with type 9, zero duration,
zero strength and zero resident source. Append that descriptor as a
buff event with attacker ID as both source and target. This effect
takes no draw, occurs before the first candidate check and can
occur with an empty world map. A zero-duration type-9 entry can
still satisfy the lookup. Action 33 bypasses this consumption site
(`0x214084–0x214178`, `0x046B15–0x046B39`,
`0x046B71–0x046B8A`).

The world creature map is traversed in ascending signed 64-bit key
order. Candidate ID comes from the resident's live `+0x08`, and the
candidate uses live health, size and position. The shared walk in
`0x214178–0x214441` rejects a null pointer, nonpositive health, an ID
already struck, a horizontal or vertical range failure or a blocked
ray before the relationship branch. Horizontal radius is base reach
plus binary32 0.7 times target size X. Vertical tolerance is target
size Z times 0.5 plus attacker size Z plus base reach. Both use
strict range comparisons. Accepted geometry goes to the live
`0x1988A0` relationship test. Hostile rolling candidates insert the
ID into deferred and skip the rest of this visit. There is no draw
on any of those early rejects or the roll deferral. Geometry failures,
existing struck membership and roll deferrals bypass the separate
32/33 route too. [Shared candidate geometry and relationships](alpha-connected-melee-candidates.md#2-candidate-set-positions-and-geometry)

For the hostile zero-roll route, the shared vector preparation from the
unshifted live positions still runs. The impulse selector `0x044860`
returns positive zero early for control kind 3 or 5 and otherwise
subtracts one and bounds through 87. Its byte/pointer tables
`0x044A38/0x044A00` use indices 29 through 32, all ordinal 8, selecting
the same zero at `0x0449FA`. The bounds fallback `0x0449D9` would
divide 2 by speed. In scope the zero scalar is applied after the
vector's size-ratio scale (`0x214441–0x214529`).

The `0x03EC90` amount coefficient subtracts 5, bounds unsigned
indices through 67, and uses the byte and pointer tables at
`0x03ED10/0x03ECF0`. The bounds fallback at `0x03ECE8` is one.
Actions 30 and 32 use indices 25 and 27 and ordinal 3, returning
binary32 0.4 at `0x03ECCA` from `0x2FD318`. Actions 31 and 33 use
indices 26 and 28 and ordinal 4, returning binary32 0.6 at
`0x03ECD4` from `0x2FD8DC`.

For equipment, `0x03DF60` first takes a category-3 subtype 6 or 7
primary `+0xAA8` alone, then checks the same priority for secondary
`+0x990`. The subsequent action selector subtracts 2, bounds the
unsigned index through 94 and uses bytes `0x03E130` and pointers
`0x03E120`. Indices 28 through 31 all use ordinal 3 and reach
`0x03E09F`, the same destination as the bounds fallback. This
takes primary if category 3, then secondary if category 3 subtype
12. It supplies the stat equipment walk and the shared hit effects.

On the hostile zero-roll path, nonzero `+0x13C0` forces critical without
a draw. With the byte clear, `0x0444A0` draws at `0x0444E5`. It tests
probability strictly above draw divided by 32,767. Resident type 11
gives probability one and still draws. Otherwise the probability is
the `0x03EA40` result plus binary32 0.15 times meter `+0x1190`.

The amount branch at `0x21452E–0x214561` chooses `0x214605` for this
group. It multiplies coefficient by the binary32 attack stat from
`0x0446E80`, then draws at `0x21463B` for 1.25 minus draw times
0.5 divided by 32,767. Multiply that variation by the previous
product, then by the binary32 sum of positive zero and one.
Critical doubles the result. The usual physical amount branch
and quadratic recorded-resource factor are bypassed. Recorded
resource and the charge indicator still go to the helper
(`0x214605–0x214694`).

Before the call at `0x21476B`, remember empty struck and attacker stun
`+0x12C` at most zero, insert target ID into struck, and set `+0x13B8`
to one. The call uses live actors, unshifted attacker position,
selected amount and critical flag, saved charge indicator and live
recorded resource, the prepared impulse and current collector. It
passes the unsigned action 30, 31, 32 or 33, an enabled group flag,
a zero after the action argument and an enabled guard flag
(`0x21469C–0x21476B`).

## 5. Hostile hit outputs and success draws

The helper `0x196D30` retests positive target health and rejects the
world-local pointer in connected mode before any helper draw or append.
Both caller writes and the preceding caller draws have already
occurred. Other early outcomes match the shared helper ordering.

| Helper branch | Order and result |
| --- | --- |
| Target kind 1 with low status byte `+0x124` mask `0x80` or contact byte `+0x5C` mask `0x20` | Draw `0x196EF9`, sound 24 at target then type-3 hit, false. |
| Guard `0x044590` admits and target guard `+0x174` is positive | Mutate target guard, mana and pending resource. Append type-1 hit, then draw `0x1971C0` and emit sound 24 at target, false. |
| Effective type-6 ward is strictly positive | Append type-5 hit, then draw `0x197478` and emit sound 91 at target. Return true if the remainder is at or below zero, otherwise continue with the remainder. |

The three sounds are volume one and pitch one plus draw times 0.3
divided by 32,767 with binary32 stages. Connected record application
is bypassed on these branches (`0x196EC1–0x197016`,
`0x19701B–0x197230`, `0x197235–0x19750B`).
[Shared protection, guard and ward state](alpha-connected-melee-actions-9-through-21.md#5-ordered-candidate-draws-direct-effects-and-success)

A continuing hit selects defense `0x0467A0` through the group flag
at `0x19750B–0x19751A`. It subtracts that defense times the shared
combo-dependent factor and floors ordered negative amount to zero.
The shared equipment effects and immediate enhancement self-heal
may follow. Then `0x197AF3` writes binary32 one to target `+0x1184`.
The group flag selects a fixed sound at the target in place of the
ordinary weapon sound branch and its `0x197D1B` draw. Specialization
one on the attacker gives sound 42, pitch one. Other specializations
give sound 39, pitch 1.5. Both have volume one
(`0x19750B–0x19754D`, `0x1975B5–0x197AC1`, `0x197AF3–0x197C55`).

Resident target type-1 entries multiply the amount in list order.
A critical hit against target kind other than 6 then draws at
`0x198258` for the species sound even when that species has no sound
entry. Afterward target signed stun strictly below minus 3,000
and kind other than 6 admit the draw at `0x198545`. It uses the
shared classifier and level rules with the passed recorded resource
seed and the target-action-84 override. A successful stun roll
emits sound 23 before the final hit append. The helper returns true
and connected mode skips final target application
(`0x198023–0x19804B`, `0x1981CB–0x1986D5`).
[Shared hit and stun details](alpha-connected-melee-candidates.md)

On true, action 30 makes a separate chance test before the usual
combo and resource success work. The call at `0x214792` supplies
binary32 0.25 to `0x0441A0`. That selector checks primary
`+0xAA8` category 3 and indexes its subtype directly, with zero
subtracted and unsigned maximum 12. Bytes at `0x044220` and
pointers at `0x044214` map subtypes 0, 1, 2 and 12 to ordinal 0,
`0x0441D5`, multiplying by 0.5. Subtypes 8 and 10 map to ordinal 1,
`0x0441C6`, multiplying by 0.3. Remaining in-range subtypes
map to ordinal 2, `0x0441E4`, the unchanged 0.25 path also used
by the bounds and category fallbacks. It always draws at
`0x0441EE` on this call. The threshold must strictly exceed the
binary32 draw divided by 32,767. This call also runs after a fully
absorbed ward that returned true (`0x0441A0–0x044211`,
`0x214770–0x21479F`).

If that chance succeeds and attacker specialization is zero, use the
type-9 resident update at `0x046AF0` with duration 30,000 ms, zero
strength and zero resident source ID. This replaces the first
matching entry or appends when absent. Append its buff event with
attacker ID as both source and target, then sound 47 at attacker
position at pitch and volume one. The specialization check follows
the chance draw. These outputs use no further draw
(`0x2147A5–0x214893`).

For all four, a true outcome increments combo only if the remembered
empty-struck and nonpositive-stun condition held. It visits
`0x0453B0` then clears `+0x74`. Independently, saved positive-resource
indicator and class 4 specialization 0 on the attacker add live
recorded resource to `+0x1190` capped at one. The connected nonlocal
gate bypasses the reward draws `0x214949` and `0x2149B4`. True
sets the per-turn success byte at `0x214A05`. False keeps the earlier
struck and latch writes (`0x214899–0x21491B`, `0x214A05–0x214A11`).

## 6. Alternate route for actions 32 and 33

The nonhostile branch at `0x214411` goes to `0x214A0E` after all early
candidate checks. The ordinary hit result also continues to the
alternate check at `0x214A11`. The checks require live action 32 or
33, the saved scratch flag from §3, a false current `0x1988A0`
hostility result and attacker specialization one. This test includes
a fresh relationship call even after the earlier relationship
decision. A target entering through the nonhostile branch has had no
roll test. That branch can therefore admit a rolling nonhostile
target or the attacker itself when it passes the earlier gates. A
nonhostile world-local candidate can also enter under the same gates.
No separate distance or actor-ID sort is applied
(`0x2143F7–0x21443C`, `0x214A0E–0x214A55`).

For each admitted target, draw first at `0x214A75` for binary32
variation 1.25 minus draw times 0.5 divided by 32,767. Read binary32
`0x0446E80` attack stat, multiply it by that variation, then by the
binary32 sum of positive zero and one, then by binary32 minus 0.1
from `0x302AC8`. The next call at `0x214AEC` uses `0x0444A0`,
which draws at `0x0444E5` under the same probability rule as in §4.
This call is unconditional on `+0x13C0`. Its result becomes the
critical byte of the record. It does not double the signed amount.

Append a type-0 hit with that signed amount, attacker and target
live IDs and the target's live fixed position. Other hit payload
fields retain the zero defaults of `0x059A30`. Then append sound 42
at the attacker's live position with pitch and volume one. These two
appends bypass the ordinary hit helper. The route makes no struck or
deferred insertion of its own and gives no combo, hit-latch or
success-byte update. Its connected application call requires both
attacker and target pointers to equal world local `+0xB8`. Since
this scope has a nonlocal attacker, it advances with the hit and
sound in their collectors and without a direct application
(`0x214A5B–0x214BB1`, `0x195A60`).

## 7. Continuation and limits

After the map walk, a true success byte permits an optional duration
read. It occurs when secondary subtype `+0x991` differs from 13 and
either primary category `+0xAA8` is zero or secondary category
`+0x990` is 3. Alternate appends alone do not set that byte. The
connected nonlocal attacker bypasses the ordinary combo timer
increment. The subsequent resident
buff pass subtracts the frame step and removes entries whose remaining
duration is at or below zero, including the consumed type-9 entry
unless replaced before that pass (`0x214BF1–0x214C2D`,
`0x211F41–0x211F56`, `0x2120E8–0x212163`, `0x2183FA–0x2184C2`).

The native speed, alternate attack and defense stat internals,
equipment enhancements, species sound lookup and ray intersection
remain shared dependencies. Input and aim producers, drawing
consumers, receipt and collector forwarding, standalone execution,
dedicated authority and nonfinite cases are outside this contract.
