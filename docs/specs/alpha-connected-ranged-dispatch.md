# Alpha connected ranged action dispatch

**FINAL / STABLE / HELD**

The supported image is original 32-bit Alpha `binary/Cube.exe`, 3,885,568
bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs at image base `0x00400000`. Action IDs, sound IDs and
table ordinals are decimal. Selector bytes, little-endian preferred
virtual-address pointers and numeric literals were read directly from
the PE.

The scope is the action dispatch within the living actor turn at
`0x20C510`, with world mode byte `+0xB4` nonzero and the actor pointer
different from world local pointer `+0xB8`. It covers the nearby ranged
entries after actions 30–33 and their other action IDs that share the
same launch or windup body. Signed-clock arithmetic below assumes
representable intermediates and a nonnegative frame step. Divided timing
assumes finite state with positive divisors.
[Preceding dispatch evidence](alpha-connected-melee-actions-30-through-33.md#1-entry-and-selector-evidence)

## 1. Dispatch selection and ownership

The original dispatch zero-extends live action byte `+0x68`, subtracts
one and bounds the unsigned index through 109 at `0x211E04–0x211E1B`.
It indexes the byte table at `0x222CCC`, then the 32-bit pointer table
at `0x222C84`. The out-of-range target is `0x215EC0`.

| Actions | Unsigned indices | Byte ordinal | Selected RVA |
| --- | --- | --- | --- |
| 22, 23, 26 | 21, 22, 25 | 2 | `0x21707A` |
| 24, 25, 27, 36 | 23, 24, 26, 35 | 3 | `0x215BB3` |
| 28 | 27 | 4 | `0x215C68` |
| 29, 35 | 28, 34 | 17 | `0x215EC0` |
| 34 | 33 | 5 | `0x214C32` |
| 37–46 and 108 | 36–45 and 107 | 6 | `0x217A21` |
| 47, 48 | 46, 47 | 7 | `0x212168` |
| 49 | 48 | 8 | `0x21548F` |
| 50, 55, 96 | 49, 54, 95 | 9 | `0x217634` |
| 89, 90 | 88, 89 | 11 | `0x2151F9` |

Actions 29 and 35 select the fallback address through an in-range
entry. The 89/90 pair is included because its windup precedes a
launch ownership gate, as with action 34's windup and hit gate.
Actions 51 and 54 select the separate shared melee body at
`0x212357`.

At the scoped ownership gates a nonzero world mode proceeds to the
pointer comparison with world `+0xB8`. A differing pointer exits
the action-specific span. At mode zero the same sites admit a
nonzero actor kind byte `+0x60` directly, otherwise comparing with
the local pointer. The gated sites tested the live pointer, with
no actor-ID comparison.

| Actions | Gate and routed exit for connected nonlocal | First excluded work |
| --- | --- | --- |
| 22, 23, 26 | `0x217087–0x217095` to `0x211F41` | Anticipation, resource commitment, launch and launch sound |
| 24, 25, 27, 36 | `0x215BC0–0x215BCE` to `0x215EC0` | Subsequent clock-bucket sound 55 body |
| 28 | `0x215C75–0x215C83` to `0x215EC0` | Entire body after the comparison |
| 34 | `0x214CF8–0x214D06` to `0x211F35` | Post-anticipation pulse, mana, hit and sound 41. Windup is earlier. |
| 37–46 and 108 | `0x217A2E–0x217A3C` to `0x211F41` | Windup draw, mana and projectile release |
| 49 | `0x21549C–0x2154AA` to `0x215EC0` | Anticipation-crossing position search |
| 50, 55, 96 | `0x217641–0x21764F` to `0x211F41` | Resource commitment, sound and pulsed projectile release |
| 89, 90 | `0x2152BF–0x2152CD` to `0x211F35` | Anticipation-crossing launch sound and projectile. Windup is earlier. |

The fallback at `0x215EC0` directly joins `0x211F41`. The continuation
from `0x211F35` also reaches that point. The nonlocal comparison there
bypasses combo timer advancement and continues to shared processing
at `0x212048`. Later common buff, clock and movement work lies beyond
this action-dispatch boundary.

## 2. Reachable windup and zero-crossing sounds

Earlier clock is the signed 32-bit value at actor `+0x6C`. Projected
clock adds the signed frame step. All the reachable sounds below take
the actor's live fixed position `+0x10`, convert the three coordinates
to binary32 blocks and append to the sound collector at `+0x08`. The
32-bit sound ID is paired with volume one and binary32 pitch equal to
one plus draw times 0.25 divided by 32,767. Each pitch operation
rounds to binary32. The named draw is from the native random slot
`0x2FC468`.

For action 34 at `0x214C32–0x214CE6` and actions 89/90 at
`0x2151F9–0x2152AD`, require earlier clock strictly below anticipation
and different signed truncated quotients of earlier and projected
absolute action clocks divided by 200. The read at `0x214C9F` for 34
or at `0x215266` for 89/90 is the one draw. It appends sound 36
before any mode and local-pointer check. A step crossing several
buckets still gives one draw and one sound. Projected clock can
reach or exceed anticipation. Earlier clock equal to anticipation
fails the windup gate. The branch has the same divisor before the
ownership comparison for either world mode.

The anticipation helper `0x03CAA0` receives minus one and reads live
action. It uses the unsigned action directly, maximum 105, no
subtraction, byte table `0x03CEB0`, pointer table `0x03CE24` and
bounds fallback `0x03CDF8`. Each result divides the stated
numerator by the binary32 product of the live `0x047700` speed and
binary32 actor `+0x17C`, then truncates the rounded quotient to
signed milliseconds.

| Action | Byte ordinal | Native target | Binary32 numerator |
| --- | --- | --- | --- |
| 34 | 17 | `0x03CC0A` | 1,600 |
| 89 | 18 | `0x03CB4B` | 600 when primary subtype byte `+0xAA9` equals 12, otherwise 1,200 |
| 90 | 34 | `0x03CDF8` | 400, through an in-range selection of the fallback target |

For 47 and 48, `0x212168–0x21217D` admits earlier clock at or below
zero and projected clock strictly above zero. The draw at `0x2121A3`
appends sound 14 at the same pitch and volume rule. The trigger uses
zero directly and has no anticipation reader or ownership check.
The positive-clock rejection and projected-zero rejection both go
through the fallback. A successful append goes to `0x211F3B`
(`0x212168–0x2121EF`).

Under the connected nonlocal predicate, these are the only
action-dispatch draws in the scoped entries. A single sound is the
entire output of each admitted visit to 34, 47, 48, 89 or 90. The
launch, hit and resource branches below are behind their respective
ownership gates.

## 3. Excluded hit pulse for action 34

The later action-34 body at `0x214D0C` requires current signed clock
strictly above anticipation A and at or below A plus middle M.
The independent `0x047310` selector uses live action via minus one,
direct unsigned action index maximum 104, bytes at `0x047628`,
pointers at `0x0475A8` and fallback `0x04757C`. Index 34 is ordinal
15 to `0x047511`, reading numerator 1,200 with the scaled-duration
rule above. The pulse period is the signed truncation of binary32
M times 0.125, with M converted to binary32 before multiplication.
It is positive in this bounded pulse path.

A pulse requires the signed truncated quotients of current clock
minus A and projected clock minus A divided by that period to
differ. There is no further upper clamp on projected clock.
The pulse body checks mana `+0x170` at least the first nominal cost
read, then subtracts a second nominal cost read before choosing
the hit target (`0x214D0C–0x214E21`).

The cost selector `0x044AE0` subtracts 3, bounds unsigned indices
through 92 and uses bytes `0x044B90` and pointers `0x044B7C`. Its
bounds fallback is positive zero at `0x044B75`. Index 31 for action
34 has ordinal 3, reaching `0x044B3B`. The pulse passes auxiliary
minus one. The cost uses the binary32 skill result of `0x03ED60`
for action 34, forms one minus that result times 0.75, then
multiplies by 0.125, rounding each operation. The skill-index
selector `0x03C9C0` subtracts 21 and bounds unsigned indices
through 82, using `0x03CA08/0x03C9F8` with fallback minus one at
`0x03C9F1`. Action 34 index 13 reads ordinal zero to `0x03C9DC`,
returning skill index 6. Its signed rank field is `+0x1150`.
For a zero rank and nonzero control kind, use signed truncation of
level `+0x190` divided by two, plus zero-extended byte `+0x1A8`
and one. Multiply binary32 rank by 0.1 and add one. Divide one
by the sum, then subtract the quotient from one. All these
floating operations round to binary32
(`0x03ED60–0x03EDDB`, `0x044B3B–0x044B72`).

The pulse looks up the 64-bit ID at `+0x1A0` through `0x02F000`
in world map `+0x04`. The lookup takes that key directly, including
zero if present, and returns null for a missing key. The pulse uses
the actor itself when the returned pointer is null. There is no
additional health, sight or scanned-candidate gate before the record
append. It consumes the amount draw at `0x214E4D` and the critical
draw through the `0x0444A0` call at `0x214EAD` in that order. The
amount is the binary32 `0x0446E80` stat times minus two, then times
binary32 1.25 minus draw times 0.5 divided by 32,767. The ordinary
critical helper draws at `0x0444E5` and returns the record critical
byte, even when actor forced-critical byte `+0x13C0` is set. The
signed amount is formed independently of that byte.
[Shared critical probability](alpha-connected-melee-actions-30-through-33.md#4-candidate-walk-and-hostile-hit-inputs)

The type-0 hit uses the actor's ID as source and the resolved or
fallback creature's live ID and fixed position as target. Append
the hit, then sound 41 at the actor's live position, pitch and
volume one. The subsequent direct application in connected mode
requires both actor and target pointer equal to world local
`+0xB8` (`0x214E21–0x214F77`, `0x195A60`). The earlier ownership
gate already suppresses all of these steps for the connected
nonlocal actor.

## 4. Excluded projectile release bodies

The scoped launch families that pass their earlier gates in other
modes append a descriptor to the world-resident list `+0x14` and
then to projectile output list `+0x20` of the collector, both using
`0x0861A0`. Keeping the two append sites distinct matters at the
ownership boundary. Neither list receives an append from those
bodies for the connected nonlocal actor.

- **Actions 22, 23 and 26.** After admission the preparation gate is
  current clock at or below A and projected clock strictly above A.
  The nominal cost lookup for these actions selects positive zero
  through ordinal 4 and `0x044B75` at indices 19, 20 and 23. The
  alternative debit therefore uses pending `+0x144`, floors ordered
  negative mana, copies pending into recorded resource `+0x13B4`
  and clears pending. The first release uses the same strict
  crossing. Action 26 has an additional release check at A plus
  300 when reached past the first release check and recorded
  resource is strictly positive. It uses current at or below that
  boundary and projected strictly above. The launch draws for
  amount at `0x2174D9`, appends to both lists, then draws once for
  the release sound, at `0x2175C4` for 22 and 23 or `0x21756C` for 26.
  Evidence is `0x21709B–0x2171DB` and `0x21747D–0x217624`.
- **Actions 37–46 and 108.** After admission, the windup path
  requires current clock at or below A minus 200 and a change of
  signed truncated 200 ms quotient in absolute clocks. It can
  draw at `0x217AB2` for sound 36. The release test requires current
  clock strictly below A plus M, current at or below its release
  boundary and projected strictly above that boundary, with
  positive squared length of live aim triplet `+0x160`. Release
  boundary is A except for 45 and 46, which use the projected
  signed pulse lower based on signed truncation of M divided by
  three, raised to A if lower. The first anticipation crossing can
  debit nominal cost with current at or below A, projected
  strictly above A and positive squared aim. Per admitted release
  the amount draw is `0x217E5A` for 108, `0x217F8B` for 37 and 43,
  `0x218028` for 45 and 46, or `0x2180C5` for 38–42 and 44.
  It precedes the resident append at `0x218159` and collector
  append at `0x218261`. There is no additional draw in the
  release-sound selection through `0x2182F5`. Evidence is
  `0x217A42–0x217AF9`, `0x217B06–0x217C77` and `0x217E0D–0x2182F5`.
- **Actions 50, 55 and 96.** The first anticipation crossing is
  current at or below A and projected strictly above A. It copies
  pending to recorded resource, subtracts pending from mana and
  clears pending, then can consume resident type 10. Release
  requires current at least A and strictly below A plus M,
  different signed truncated quotients of absolute current and
  projected clocks divided by signed truncation of M divided by
  four, and positive squared aim. Draw at `0x217812` for sound 22
  first, then `0x2179AB` for amount before the appends at
  `0x2179FC/0x217A11`. Evidence is `0x217655–0x2177F2` and
  `0x2177F8–0x217A1C`.
- **Actions 89 and 90.** The separate release gate requires
  current at or below A and projected strictly above A. The
  `0x215321` draw appends sound 38 with the windup pitch rule
  before both projectile appends. This draw occurs after any
  admitted windup draw in the same visit. The projectile
  descriptor uses the actor's live source ID and fixed position,
  a zero velocity triple, 32-bit type 3 at descriptor `+0x60`,
  and variant byte `+0x64` equal to 1 for 89 or 2 for 90. The
  amount is `0x0446E80` attack stat times the selected coefficient,
  then times two, with binary32 stages and no amount draw. The
  `0x03EC90` coefficient selector subtracts 5 and bounds through
  unsigned index 67. Both indices 84 and 85 reach the bounds
  fallback at `0x03ECE8`, returning one before byte or pointer
  lookup. The resident and collector appends are at
  `0x215470/0x215485`. Evidence is `0x2152D3–0x21548A` and
  `0x03EC93–0x03ECA5`.

The nonlocal actor returns before these release and pulse timing
reads. The nearby ranged entries therefore can contribute received
actor sounds without performing the launch-side mutations in those
bodies. The detailed launch descriptors beyond the 89/90 fields
above, world projectile receipt and stepping, later generic actor
work, full local or standalone behavior and dedicated authority
remain outside this contract.
