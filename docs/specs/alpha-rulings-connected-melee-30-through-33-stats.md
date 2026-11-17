# Alpha connected melee actions 30 through 33 stat ruling

**FINAL / STABLE / HELD**

The supported original image is the 32-bit Alpha `binary/Cube.exe`, 3,885,568
bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
with preferred image base `0x00400000`. Addresses are client RVAs unless
identified as preferred virtual addresses. This ruling covers finite native
state, representable arithmetic and positive divisors in the connected
nonlocal common-action paths for actions 30 through 33. It resolves the
attack-reader address in sections 4 and 6 and supplies the stat numerics
for sections 4 through 6. [Connected melee actions 30–33](alpha-connected-melee-actions-30-through-33.md)

## Call sites and use of the results

| Use | Original call site RVA | Reader RVA | Reader preferred VA |
| --- | --- | --- | --- |
| Hostile amount for actions 30–33 | `0x214618` | `0x046E80` | `0x00446E80` |
| Alternate heal for actions 32 and 33 | `0x214AAC` | `0x046E80` | `0x00446E80` |
| Continuing hostile hit defense for actions 30–33 | `0x197513` | `0x0467A0` | `0x004467A0` |

Both amount paths call the same creature attack reader on the live
attacker. The hostile branch at `0x21452E–0x21455B` reaches
`0x214605` with the live action byte. It first gets the `0x03EC90`
coefficient, then multiplies it by the attack result. The coefficients are
binary32 0.4 for actions 30 and 32 and 0.6 for actions 31 and 33.
The byte and pointer tables at `0x03ED10` and `0x03ECF0` select
`0x03ECCA` and `0x03ECD4`, respectively. It draws at `0x21463B`.
Convert the signed draw to binary32, multiply it by 0.5, divide by
32,767 and subtract from 1.25. Multiply that variation by the
coefficient/stat product, then multiply by the separately formed sum
of positive zero and one. A saved critical decision doubles that amount.
The hostile branch uses these scalar factors without a recorded-resource
term. Evidence is `0x214605–0x214694`.

The alternate route checks live action 32 or 33, the saved pulse condition,
current nonhostility and attacker specialization byte `+0x141` equal to
one at `0x214A11–0x214A55`. It draws at `0x214A75` and forms variation
by the same binary32 steps before reading the attack stat. It multiplies
the stat by that variation, then by the sum of positive zero and one, then
by binary32 minus 0.1 from `0x302AC8`. This path uses the stat directly,
with neither an amount coefficient nor a recorded-resource term. The later
`0x0444A0` call at `0x214AEC` sets the record's critical byte without a
magnitude change, regardless of the forced-critical latch. The route
appends its signed hit and sound outside the generic hit helper. Its
connected nonlocal continuation therefore has no target defense sample
or direct application of that hit (`0x214A5B–0x214BB1`).

The hostile call to `0x196D30` at `0x21476B` supplies group flag one for
these actions via `0x2146F0–0x214714`. The helper applies the earlier health,
connected local-target, protection, guard and ward gates before the
defense block. At `0x19750B–0x19751A` the nonzero group flag selects
the defense reader with the target as receiver. The target receiver is
established at `0x196D73`. The caller uses the remaining amount after
ward, or the original constructed amount when the ward stage leaves it
intact. It subtracts the rounded product of the saved combo-derived
factor and the rounded defense result and floors an ordered negative
remainder to positive zero (`0x1974E2–0x19754D`).

## Shared level, rank and item power arithmetic

Each scalar operation below is binary32, with a rounding boundary per
operation. For creature level, convert the signed 32-bit value at
`+0x190` to binary32. Item queries supply a binary32 effective level.
Subtract one from the level, multiply the difference by binary32 0.05,
then add one. Divide one by that sum, subtract the quotient from one
and multiply by three. Widen this exponent to binary64 and use it for
the imported power of binary64 two. Narrow the result to binary32 to
obtain the level factor.

The rank factor comes from a second power of binary64 two. Convert the
rank integer where read to binary32, multiply by binary32 0.25, widen
the exponent and narrow the power result to binary32. Creature rank is
the unsigned byte at `+0x1A8` when a branch reads it. Item power at
`0x045F60` multiplies the rounded level factor by the rounded item rank
factor and returns the stored binary32 product. The native calls use
`_libm_sse2_pow_precise` through `0x28D8D4` and import slot `0x2FC454`.
The level literal 0.05 is `0x3D4CCCCD` at `0x2FCDB4` and the exact quarter
is at `0x345D98`. Evidence includes `0x045F60–0x045FE5`,
`0x046EAA–0x046F33` and `0x0467B4–0x046836`.

Both item readers scale signed 32-bit item `+0x114` by binary32 0.1
after integer conversion. They combine that term with signed 16-bit
item `+0x10` converted to binary32 to obtain the effective level.
Weapon addition has the converted `+0x10` term first. Defense-item
addition has the scaled `+0x114` term first. They both supply unsigned
item rank byte `+0x0C` to `0x045F60`. Binary32 0.1 is `0x3DCCCCCD`
at `0x2FCB7C`. The power result is narrowed before each item multiplier.
Evidence is `0x0C7F71–0x0C7FD6`, the corresponding weapon continuations,
and `0x0C7B5B–0x0C7BA8`.

## Attack accumulation and weapon order

The creature base at `0x046EAA–0x046F41` uses the level factor and the
rank power call with exponent formed by multiplying binary32 0.25 by
positive zero. Multiply this rank result by the saved level factor, then
use the binary32 coefficient at attacker `+0x180` as the first operand
of multiplication by that product. This bases the initial contribution
on rank zero for every control kind. The native reader executes both
power calls before obtaining the weapon list.

The list reader `0x03DF60` first selects primary `+0xAA8` alone if its
category is 3 and its subtype is 6 or 7. Otherwise it gives the same
priority to secondary `+0x990`. With neither priority match, actions
30–33 all select ordinal 3 at `0x03E09F`, using live action minus two,
byte table `0x03E130` and pointer table `0x03E120`. In order, include
primary if category 3 and secondary if category 3 with subtype 12.
The stat calls `0x0C7F60` for each selected item in list order and adds
the returned binary32 item value as the first operand to the current
accumulator. Both selected slots contribute their full item values.
Evidence is `0x03DFA9–0x03E0D3` and `0x0470A8–0x0470D3`.

The weapon item reader returns positive zero for categories other than
3. A category-3 item multiplies its rounded power by the following
binary32 factor, selected using the unsigned subtype at item `+0x01`.

| Weapon subtypes | Item multiplier |
| --- | --- |
| 3, 4, 13 | 2 |
| 6, 7, 8, 10, 11, 15, 16, 17, 18 | 8 |
| All remaining subtypes, including 5 and 12 | 4 |

The selector at `0x0C7F7F–0x0C7F9D` subtracts three and bounds the
unsigned result through ten. Tables `0x0C811C` and `0x0C810C` give
`0x0C7FA4` for 3 and 4, `0x0C7FE2` for 5, `0x0C8020` for 13, and
`0x0C805E` for 6–12. The bounds fallback also reaches `0x0C805E`.
Its comparisons select factor eight at `0x0C80CE` or factor four at
`0x0C8090`. Subtype 5 reaches its factor-four table entry before the
fallback comparisons. Evidence ends at `0x0C810B`.

With an empty weapon list, attacker status word `+0x7E` supplies two
independent contributions. Mask `0x08` adds the product of a fresh level
factor and fresh actual-rank factor, multiplied by two. Mask `0x10` then
adds a new level/rank product multiplied by eight. Both can contribute,
in that order. Each added term is the first addition operand and uses
no creature coefficient. A nonempty weapon list takes the item walk.
The stored binary32 accumulator is the return at `0x0470F7`, following
list cleanup. Evidence is `0x046F46–0x0470A6` and `0x0470D5–0x04710E`.

## Defense base and equipment order

The defense reader first evaluates the level and actual-rank powers
for the target, then multiplies the rounded rank factor by the rounded
level factor. Save this product and multiply the binary32 defense
coefficient at target `+0x188` by it as the provisional base.

When target control byte `+0x60` is zero, a third power call takes
binary64 two with binary64 exponent one from `0x345DF8`. Its narrowed
result is multiplied first by the saved level factor and then by the
coefficient at `+0x188`, replacing the provisional base. The saved
actual-rank/level product survives this replacement. For any control
kind, target `+0x7E` mask `0x20` adds that saved product to the current
base, with the base as the first addition operand. These steps precede
the equipment loop (`0x0467B4–0x046894`).

The defense reader considers the following slots in order. Each slot
qualifies on exact category-byte equality. It queries `0x0C7AF0` for
the admitted item, stores the return as binary32 and adds it as the
first operand to the running result. It returns the stored binary32
total (`0x046894–0x046942`).

| Target slot | Required category | Item reader call RVA |
| --- | --- | --- |
| `+0x530` | 4 | `0x0468A3` |
| `+0x648` | 6 | `0x0468CE` |
| `+0x760` | 5 | `0x0468F9` |
| `+0x878` | 7 | `0x046925` |

Defense item weight starts at one for category 4 and one half for
categories 5, 6 and 7. Other categories return positive zero from the
item reader. Unsigned material byte `+0x0D` multiplies the starting weight
by binary32 0.85 for 1 and 19 or by binary32 0.75 for 26 and 27.
The remaining materials preserve the starting weight. The rounded item
power is the first operand of the final multiplication by this weight.
The material selector subtracts one and bounds through 26, then uses
`0x0C7BB8` and `0x0C7BAC`. Its two weighted targets are `0x0C7B44` and
`0x0C7B4E` and its unchanged target and bounds fallback are `0x0C7B5B`.
Binary32 0.85 is `0x3F59999A` at `0x302D4C` and 0.75 is exact at
`0x2FCDC4`. Evidence is `0x0C7AF0–0x0C7BA8`.

## Defense scaling in the hit helper

The saved factor for ward and defense is binary32 one minus the attacker
combo fraction. Compute that fraction by converting signed combo
`+0x70` and the signed cap from `0x044CB0` to binary32 and dividing.
An ordered first fraction strictly above one supplies one. Otherwise the
helper reads the combo and calls the cap reader again, then uses the
recomputed fraction, including negative values. Evidence is
`0x197267–0x1972DF`.

The cap reader multiplies attacker binary32 `+0x17C` by its selected
numerator and truncates toward zero to a signed 32-bit integer.
With category 3 at `+0xAA8`, subtype 6 uses 60, subtype 8 uses 80,
subtypes 11, 12, 15, 16 and 17 use 20, and other subtypes use 50.
When that slot has another category, mask `0x10` at `+0x7E` selects
20 and the clear-mask path selects 50. Evidence is `0x044CB0–0x044D28`
and the table at `0x044D2C`. The defense stage uses the saved factor
from before ward processing, even when it is zero.

## Evidence and limits

The original image confirms the three relative call targets directly.
The checked instruction export matches 846 entries in the scoped
callers, queries and helpers. Direct disassembly supplies the attack
reader's omitted cleanup and return at `0x0470E8–0x04710E`. Switch
entries, numeric literals and the native power and draw import slots
were read from the image. The two stat readers and their covered item
and equipment dependencies contain no game random draw. The branch
draws at the named caller sites use the original `rand` import at
`0x2FC468`.

These relations fix arithmetic stages and rounding boundaries. Imported
power results retain the original runtime's last-bit behavior. Unusual
floating-point environments, nonfinite or unrepresentable inputs,
zero divisors, other hit effects and server and runtime acceptance
remain open.
