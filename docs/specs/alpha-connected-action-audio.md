# Alpha connected-client common action audio and target-window preparation

Status **FINAL / STABLE / HELD**

## Scope

The source is the original 32-bit Alpha client `binary/Cube.exe`,
3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
image base `0x400000`. Code addresses are client RVAs. The original
dispatch is at `0x211E04–0x211E1B` with tables at `0x222C84` and
`0x222CCC`. The scoped common body starts at `0x212357`. This
contract ends where its current candidate traversal starts at
`0x214178`. The listed sounds and events are native output records.

A connected nonlocal actor is on the path with world byte `+0xB4`
nonzero and differs from the local creature pointer at world `+0xB8`.
The main world turn admits it after its positive-health check.
The common body accepts the nonzero world mode directly. The earlier
action-lock phase can already have overwritten the actor clock at
`+0x6C` with its current full duration plus one. The audio phase uses
the clock after that preliminary phase and before the common
subtraction from stun or the normal clock increment. Evidence is
`0x211A96–0x211AE1`, `0x211D0D–0x211D4E`, and `0x212357–0x2123A9`.

Action numbers and sound IDs below are decimal. The action is a byte
at actor `+0x68`. Let the current clock be the live signed 32-bit
value at `+0x6C` and elapsed be the signed integer milliseconds of the
world step. The current anticipation result is returned by `0x03CAA0`,
middle duration by `0x047310`, and total duration by `0x03D1A0`.
Where a clock boundary is calculated again later, it uses then-current
inputs. Comparisons and clock arithmetic are signed. Quotients used
for buckets truncate toward zero. The supported domain here is finite,
valid actor state, nonnegative elapsed time, positive cadence
denominators and representable arithmetic.

## 1. Sound record and early ordinary sound

Each named sound record uses the current actor position from `+0x10`
at that site. For each axis, `0x02C4A0` converts the signed 64-bit
fixed coordinate to binary32 before multiplying by binary32
`0x37800000` (one divided by 65,536). The ID, pitch and volume are
at record offsets `+0x0C`, `+0x10` and `+0x14`. The constructor
`0x065FB0` sets pitch and volume to one. Every sound described here
keeps volume one and appends via `0x0864C0`.

Each randomized pitch below consumes one integer result of the same
`MSVCR110.rand` import at `0x2FC468`. For the listed pitch base and
multiplier, convert the draw to binary32, multiply by the listed
binary32 multiplier, divide by binary32 32767, then add the
listed binary32 base. Each stage rounds separately. Extra scaling
for the shared swing is applied after those stages. Evidence for
constants is `0x2FCB7C` (0.1), `0x2FCDBC` (0.2), `0x345D98` (0.25),
`0x2FD338` (32767), `0x2FD31C` (0.9), `0x345DA0` (0.5) and
`0x345DC0` (1).

The held-item predicate at `0x044230` requires actor byte `+0xAA8`
equal to 3 and subtype `+0xAA9` in 5, 6, 7, 8, 10, 11, 15, 16,
17 or 18. An admitted common action outside 30–33, 87, 88 and 91
with clock exactly zero and this predicate true emits sound 17,
with base 1 and multiplier 0.1. The draw at `0x212442` precedes
append. The entry write of 600 to roll `+0x128` for class 4,
specialization 1, actions 5, 17 or 20 with clock exactly zero
can already have happened. It leaves this sound gate eligible.
Evidence is `0x212378–0x212495` and `0x044230–0x04426C`.

## 2. Initial sound and effect routes for 30–33, 87 and 88

Anticipation for these six actions divides a binary32 numerator
by the binary32 product of the timing-factor result at `0x047700`
and actor binary32 `+0x17C`, then truncates toward zero to signed
integer milliseconds. The timing-factor return rounds to binary32
before multiplication.

| Actions | Anticipation numerator | Middle duration |
| --- | ---: | --- |
| 30, 32 | 800 | Fixed signed integer 600. |
| 31, 33 | 1600 | Fixed signed integer 1200. |
| 87 | 5000 | Binary32 300 divided by the timing product, then truncation. |
| 88 | 400 | Binary32 300 divided by the timing product, then truncation. |

The timing selectors at `0x03CE24`, `0x03CEB0`, `0x0475A8` and
`0x047628` lead to `0x03CBD6`, `0x03CBF0`, `0x03CAFD`, `0x03CDF8`,
`0x04743F`, `0x047507` and `0x04757C`. The arithmetic tails are
`0x03CE0D–0x03CE20` and `0x047589–0x0475A4`.

The two groups have the same first periodic sound. When current
clock is **strictly below** anticipation, it compares the signed
quotient of clock divided by 50 with the signed quotient of
clock plus elapsed divided by 50. A difference appends one
sound 36 with base 1 and multiplier 0.25. This is one draw even
when multiple quotients are crossed. The draw is at `0x212502`
for 30–33 and `0x212809` for 87/88. Both paths then separately
test their deterministic emitters. Evidence is `0x212495–0x212552`
and `0x21279C–0x212859`.

For 30–33, the deterministic pair requires current clock **strictly
below** anticipation and projected clock **at least** anticipation.
First append sound 41 if specialization byte `+0x141` is 1, otherwise
sound 38. Use unit pitch. Then append the visual record. The pair
itself draws no random number. The visual position is fixed actor
position plus live aim `+0x160` converted independently per component
by binary32 multiplication by 65,536 and truncation to signed 64
bits. The addition is fixed point. The sound uses the original
unshifted actor position.

For 87 and 88, the deterministic pair instead requires current
clock minus elapsed **strictly below** anticipation and current
clock **at least** anticipation. First append sound 81 at unit
pitch, then append the visual record. Both use the unshifted
actor position. The pair itself draws no random number.

| Visual payload field | Actions 30–33 | Actions 87 and 88 |
| --- | --- | --- |
| Fixed-point position at `+0x00` | Actor position plus converted live aim. | Actor position. |
| Binary32 vector at `+0x18` | (0, 0, 2). | (0, 0, 2). |
| Four binary32 color components at `+0x24` | Explicit (0, 0.2, 1, 1) write when specialization is 1. | No explicit color store in the producer. |
| Binary32 `+0x34` | 0.25. | 0.25. |
| Signed integer `+0x38` | 20. | 80. |
| Signed integer `+0x3C` | 2 if specialization is 1, otherwise 1. | 1. |
| Binary32 `+0x40` | 8. | 20. |

The visual constructor `0x065ED0` only supplies defaults at `+0x3C`
and `+0x40`, which these routes replace. The outgoing visual append
via `0x086470` copies the record. The color storage on the 30–33
branch without specialization 1 also has no explicit color store
in this producer. Its value and the consumer's interpretation of
unassigned color storage are outside the evidence. The payload
fields shown as assigned above are independent of candidate
selection. Evidence is `0x212552–0x2126E1`, `0x212859–0x21295A`,
`0x02C460`, `0x02C800`, `0x065E60–0x065EE0` and `0x086470`.

## 3. Shared swing and middle cues

The ordinary and special routes meet at `0x2126F3`. The shared swing
requires current clock **at most** anticipation and clock plus
elapsed **strictly above** anticipation. It compares to the
anticipation value initially obtained at `0x2123AD`. Action 91
skips the shared swing. The common actor's local-pointer equality
is not required. All remaining members reaching this gate take
one draw at `0x212738` and start with base 0.9 and multiplier 0.2.

| Action | Sound and scaling after the standard pitch stages |
| --- | --- |
| 10 | Sound 16, with the base-pitch result unchanged. |
| 11 | Sound 48, with the base-pitch result unchanged. |
| 54 | Sound 14, with the base-pitch result unchanged. |
| Other admitted common action except 91 | Sound 15. Multiply the rounded pitch by binary32 1.5. Then multiply that product by binary32 0.5 if the held-item predicate is true or action is 68, 69 or 93. |

The append follows the initial periodic and deterministic pairs
when those are eligible. The pitch scaling uses two separate
binary32 multiplications on the route with both. The constant 1.5
is at `0x2FCDE0`. Evidence is `0x2126E7–0x212791`,
`0x21297B–0x212A66` and `0x044230`.

After the swing test, actions 57, 58, 60 and 74 can emit sound 12,
with base 1 and multiplier 0.25, when clock is strictly below
anticipation plus one half of middle and clock plus elapsed is
at least that threshold. The middle half is signed integer
division truncated toward zero. The draw is at `0x212AFD`.
Action 74 tests the midpoint-crossing condition again later at
`0x213200–0x21325A`, after the at-or-before-anticipation clearing
pass. That later cue is sound 82, with base 0.5 and multiplier
0.1, with its own draw at `0x21327A`. Each midpoint test queries
the timing helpers at its own site. Evidence for the first cue
is `0x212A66–0x212B43` and for the second is `0x213200–0x2132CC`.

## 4. Repeating phase and the saved action-31 window

After the preceding swing and first midpoint checks, actions
5, 11, 30–33, 72, 86 and 91 use a separate bound producer.
The general provisional bounds at `0x212B43–0x212C05` are replaced
on this route. For a current clock below anticipation or at
least anticipation plus middle, set both saved bounds to the
current anticipation result and skip the repeat sound check.
A distinct, later target gate uses a strict lower comparison,
so these equal bounds form an empty target window.

For current clock at least anticipation and strictly below
anticipation plus middle, the period is as follows.

| Action | Period in signed milliseconds |
| --- | --- |
| 30, 32 | Middle divided by 6, which is 100 in the supported image. |
| 31, 33 | Middle divided by 12, which is 100 in the supported image. |
| 5, 11 | Middle divided by 3. |
| 72, 86 | 250. |
| 91 | 1000. |

The saved lower bound is anticipation plus period times the signed
quotient of current clock minus anticipation divided by period.
The upper bound is that lower bound plus **100 ms**, including for
actions whose period differs from 100. The same lower-bound calculation
is made separately with clock plus elapsed. If the projected lower
bound differs from the saved lower bound and is strictly below
anticipation plus middle, copy the combo count `+0x70` to
`+0x1314` and clear the struck and stored-candidate collections at
`+0x11AC` and `+0x11B4`. This clear does not run the type-4 feedback
pass and has no direct write to `+0x13B8`. It sets a temporary
condition later read on selected candidate paths. The temporary
condition is also set if clock plus elapsed is strictly above the
saved upper bound. The projected computation leaves the saved
bounds unchanged. Evidence is `0x212CB2–0x212EF2`,
the equal-bounds route at `0x213037–0x213052` and the later
read at `0x214A26`.

Action 5 skips this route's audio after the state and bounds work.
The other listed repeating actions use one of two conditions for
the repeat sound. Either current clock minus elapsed is strictly
below anticipation, or the signed period quotients of
clock minus anticipation and clock plus elapsed minus anticipation
differ. On admission, append sound 15 using base 1 and multiplier
0.25, with one draw at `0x212F7C`. Action 91 immediately appends
a second sound, 82, using base 0.5 and multiplier 0.1 with a
separate later draw at `0x212FEF`. Both use the current actor
position. No shared-swing pitch scaling applies to this repeat
sound. Evidence is `0x212EF8–0x213035`.

The bound behavior for action 31 holds before the later
counter-and-clock adjustment at `0x213448–0x2134EE`. That adjustment
can replace the live clock as a result of clearing the combo
count. It leaves the saved bounds from the earlier calculation in
use. After the charge debit and pending-resource phase, the
target gate at `0x2137F7–0x213808` reads the **live signed
clock** and admits it only when strictly above the saved lower
bound and at most the saved upper bound. It does not project the
clock again there. The charge debit independently uses its own
forward crossing out of inclusive anticipation at
`0x213754–0x2137D4`. An action-31 turn can therefore spend or emit
sound before reaching an empty or failed target window.

## 5. Deterministic output before the candidate traversal

The earlier stored-candidate cleanup can emit even when the
upcoming current candidate traversal admits nobody. At or below
anticipation, it copies the combo count to `+0x1314`, then checks
the recorded IDs in `+0x11B4`. An ID absent from the struck set
`+0x11AC` and resolving to a creature receives the dodge-benefit
call and a type-4, zero-amount hit record. The record uses the
acting creature and resolved creature IDs and the latter's
current position. After that traversal both collections are
cleared and `+0x13B8` becomes zero. A second earlier-collection
cleanup at or above its saved expiry threshold can emit the
same type-4 records and clears only `+0x11B4`. The latter uses
the saved upper bound when action 31 takes the bounded phase
route of section 4. The dodge benefit itself requires a
nonzero target roll and class 4, specialization 1. It adds
binary32 0.25 mana with upper cap one, replaces or creates
resident type 11 for 30,000 ms, and appends that target's buff
event before the type-4 record. The type-4 append still occurs
when the benefit predicate fails. Neither cleanup requires a
draw at these sites. Evidence is `0x213052–0x213200`,
`0x2132CC–0x213448` and `0x03CF20–0x03CFE1`.

The ordinary resource initializer can also emit before the
current target gate. Actions 5, 17 and 20 at an eligible zero
clock with resident type 11 replace its first record with
zero strength, duration and resident source ID, append a
self-addressed buff event, and then set actor byte `+0x13C0`
to one. Evidence is `0x21367D–0x21374F` and `0x046AF0`.

After terminal admission to the target window, the connected
mode jumps at `0x213B1E–0x213B25` to `0x214084`. The intervening
world-byte-zero event and terrain work, including its draw at
`0x213EA9`, is outside the connected path. At the continuation,
action 31 reads current anticipation again. If live clock is
at least that result and resident type 9 is found, it replaces
the first matching record with type 9, binary32 positive-zero
strength, zero duration and zero 64-bit resident source ID.
The outgoing buff event uses the actor's 64-bit ID in both
endpoint fields and appends via `0x0460A0`. This occurs before
the candidate iterator is acquired at `0x214178`, and takes no
random draw. The later buff expiration phase can remove the
zero-duration replacement. This is a separate event from the
earlier nominal debit, which tests resident presence before
this replacement. Evidence is `0x2140AE–0x214178`, `0x02C520`,
`0x043F10`, `0x046AF0` and `0x212130–0x212163`.

## 6. Boundary examples

Use action 31, a timing product of one, and unchanged time
inputs until after the named comparisons. The anticipation
is 1600 and the period is 100.

- Clock 1580 with elapsed 20 takes sound 36, then the deterministic
  launch sound and visual record. The shared swing and charge
  debit crossing tests fail at projected equality. The repeating
  phase is inactive and its saved bounds both equal 1600.
- Clock 1600 with elapsed 20 can take the shared swing, then the
  separate repeat sound because clock minus elapsed precedes
  anticipation. Each sound takes its own draw. The saved range
  has lower bound 1600 and upper bound 1700. The unchanged
  live clock equals the lower bound, so no current candidate
  iteration or its action-31 type-9 replacement follows.
- Clock 1699 with elapsed 1 saves the same 1600 and 1700 bounds,
  clears the stored sets on the projected new phase and can
  pass the later target window using live clock 1699. A turn
  starting at exact clock 1700 instead saves lower bound 1700
  and fails the strict lower comparison if live clock stays
  there.

For actions 87 and 88 with positive elapsed, a current clock
equal to anticipation can emit the deterministic 81 sound
and its visual record, then the shared swing. Their periodic
36 gate has already failed at that exact current-clock equality.

## Limits

The audit covers the named original client's pre-iteration
sites. The full candidate geometry, later action branches,
post-iteration outputs, event playback and network delivery
remain open. Color reads outside the producer, full stat
formulas, malformed state and server behavior are outside
this contract. Evidence is static.
