# Alpha critical and haste stat numerics

**FINAL / STABLE / HELD**

## Supported images and scope

| Original file | Size in bytes | SHA-256 |
| --- | ---: | --- |
| `binary/Cube.exe` | 3,885,568 | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| `binary/Server.exe` | 1,718,784 | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

Both PE32 images have preferred base `0x00400000`. Addresses in the
contract are image-relative. Each row below gives the corresponding
reader in both images. The statements cover those readers and the named
common-action conditions once their sites are reached.

| Reader | Client RVA | Server RVA |
| --- | --- | --- |
| Critical decision | `0x0444A0` | `0x00F520` |
| Creature critical contribution | `0x03EA40` | `0x009AC0` |
| Item critical contribution | `0x0C6BA0` | `0x013AC0` |
| Total attack speed | `0x047700` | `0x012150` |
| Creature haste contribution | `0x0478B0` | `0x012300` |
| Item haste contribution | `0x0C7C00` | `0x014350` |
| Normalized item power | `0x045FF0` | `0x010F90` |
| Combo cap | `0x044CB0` | `0x00FCF0` |

## Shared conventions and exact constants

Creature and item inputs are live record pointers. Stat and speed
queries return stored binary32 values through the x87 return stack.
Each binary32 operation rounds before the next. A completed power
exponent is widened to binary64, and each imported result is narrowed
before subsequent game-owned arithmetic.

| Decimal use | Format | Native bits | Client RVA | Server RVA |
| --- | --- | --- | --- | --- |
| Fixed rank input and positive zero | Binary32 | `0x00000000` | `0x345D84` | `0x158698` |
| One | Binary32 | `0x3F800000` | `0x345DC0` | `0x1586DC` |
| Level coefficient and lower critical-item coefficient, 0.05 | Binary32 | `0x3D4CCCCD` | `0x2FCDB4` | `0x1586B8` |
| Creature scale, higher critical-item and lower haste-item coefficient, 0.1 | Binary32 | `0x3DCCCCCD` | `0x2FCB7C` | `0x1586BC` |
| Critical meter coefficient, 0.15 | Binary32 | `0x3E19999A` | `0x2FCDB8` | `0x1586C0` |
| Higher haste-item coefficient, 0.2 | Binary32 | `0x3E4CCCCD` | `0x2FCDBC` | `0x1586C4` |
| Level exponent scale, 3 | Binary32 | `0x40400000` | `0x2FCD70` | `0x158718` |
| Rank exponent coefficient, 0.25 | Binary32 | `0x3E800000` | `0x345D98` | `0x1586C8` |
| Combo speed multiplier, 0.5 | Binary32 | `0x3F000000` | `0x345DA0` | `0x1586D0` |
| Nonzero-kind speed baseline, 0.75 | Binary32 | `0x3F400000` | `0x2FCDC4` | `0x1586D4` |
| Nonzero-kind speed rank step, 0.0625 | Binary32 | `0x3D800000` | `0x2FD8D0` | `0x1588B0` |
| Seed remainder divisor, 20 | Binary32 | `0x41A00000` | `0x2FCD74` | `0x15874C` |
| Item filter threshold, 0.001 | Binary32 | `0x3A83126F` | `0x2FD870` | `0x158858` |
| Critical draw divisor, 32,767 | Binary32 | `0x46FFFE00` | `0x2FD338` | `0x158834` |
| Power base, 2 | Binary64 | `0x4000000000000000` | `0x345E18` | `0x1588D8` |
| Fixed normalization exponent, 3 | Binary64 | `0x4008000000000000` | `0x2FCB98` | `0x1588E0` |
| Selected seed-conversion correction, positive zero | Binary64 | `0x0000000000000000` | `0x345F30` | `0x15AC20` |

Decimal labels identify the stored values. Both images import
`MSVCR110.DLL::_libm_sse2_pow_precise` through the respective thunks
`0x28D8D4` and `0x14AB14`. The direct import slots are client `0x2FC454`
and server `0x1582F8`. The native caller prepares binary64 base and
exponent in the low scalar halves of the first two vector registers,
with positive-zero upper halves at these sites. The low binary64 result
in the first register is narrowed locally.

## Initial creature contribution: level, rank and role

The critical and haste contribution readers share the same initial
arithmetic. Convert signed 32-bit creature level at `+0x190` to
binary32. Subtract one, multiply the difference by 0.05, then add one.
Divide one by that sum, subtract the quotient from one, and multiply
the difference by three. Widen the exponent and evaluate a binary64
power of two. Narrow the result to binary32 as the level factor.

Form the second exponent by multiplying binary32 0.25 by positive
zero. Widen it and evaluate a second binary64 power of two. Multiply the
saved binary32 level factor by the narrowed rank factor. Divide that
stored product by the narrowed result of a third power call, using
binary64 two and binary64 exponent three. Then multiply the quotient
by binary32 0.1. This initial contribution uses fixed rank input zero
for every creature kind and is independent of the class, action and
resident statuses. All three power calls occur for this contribution.

| Caller | Client power call RVAs in order | Client result narrowing RVAs in order | Server power call RVAs in order |
| --- | --- | --- | --- |
| Creature critical | `0x03EA8A`, `0x03EAB6`, `0x03EAE0` | `0x03EA92`, `0x03EABE`, `0x03EAF1` | `0x009B0A`, `0x009B36`, `0x009B60` |
| Creature haste | `0x0478FA`, `0x047926`, `0x047950` | `0x047902`, `0x04792E`, `0x047961` | `0x01234A`, `0x012376`, `0x0123A0` |
| Normalized item power | `0x04602E`, `0x046057`, `0x046081` | `0x04603B`, `0x04605F`, `0x04608B` | `0x010FCE`, `0x010FF7`, `0x011021` |

## Eligible creature slots and sequential additions

Both creature contribution readers test exactly these category bytes in
this order. A mismatching category skips the slot. Each accepted slot
calls the corresponding item reader and stores its return as binary32.
The item value is the first operand of the addition and the current total
is the second. A returned positive zero still goes through the addition.

| Addition order | Creature-relative record start | Required category |
| --- | --- | --- |
| 1 | `+0x990` | 3 |
| 2 | `+0xAA8` | 3 |
| 3 | `+0x530` | 4 |
| 4 | `+0x648` | 6 |
| 5 | `+0x760` | 5 |
| 6 | `+0x878` | 7 |
| 7 | `+0x418` | 8 |
| 8 | `+0xBC0` | 9 |
| 9 | `+0xCD8` | 9 |

The gates admit both category-3 slots and both category-9 slots
independently. The final result is the last accumulated total. The item
threshold and coefficient belong to each item helper. A normal creature
query executes three imported power calls for its initial term plus
three for each admitted slot, or 30 when all nine qualify. Evidence is
client `0x03EB11–0x03EC89` and `0x047981–0x047AF9`, and server
`0x009B91–0x009D09` and `0x0123D1–0x012549`.

## Item inputs, category gates and raw fields

Both item helpers admit categories 3 through 9. An unadmitted category
returns positive zero before the other field reads or power calls.
Admitted items use the following values.

| Item-relative offset | Read |
| --- | --- |
| `+0x00` | Category byte for eligibility and coefficient selection |
| `+0x01` | Subtype byte when category 3 |
| `+0x04` | Unsigned 32-bit seed |
| `+0x0C` | Unsigned rank byte, passed as a zero-extended integer |
| `+0x0D` | Material byte compared for equality |
| `+0x10` | Signed 16-bit raw level, converted directly to binary32 |

The normalized power inputs come from the raw item level and rank. The
extra-block count at `+0x114` is unused in these readers. Level retains
its sign and rank retains the full unsigned byte range. The outer
creature slot predicates and inner item category gates are the eligibility
checks in this contract. Evidence spans client `0x0C6BA0–0x0C6CBD` and
`0x0C7C00–0x0C7D12`, and server `0x013AC0–0x013BDD` and
`0x014350–0x014462`.

## Item coefficients and seed/material factors

The higher coefficient applies to category 4 or to category 3 with
subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18. The lower coefficient
applies to the remaining admitted items.

| Contribution | Lower coefficient | Higher coefficient | Factor from the remainder quotient |
| --- | --- | --- | --- |
| Critical | 0.05 | 0.1 | Subtract the quotient from one, then add one for material 11 |
| Haste | 0.1 | 0.2 | Use the quotient, then add one for material 12 |

Divide the raw unsigned seed by integer 21 and retain the remainder
from 0 through 20. Convert that remainder to binary64 and add the
positive-zero correction at index zero of the native conversion table.
Narrow to binary32 and divide by binary32 twenty to obtain the quotient.
The material increment, when selected, is a separate binary32 addition.
The remainder is deterministic and consumes no game random draw.
Evidence is client `0x0C6BD1–0x0C6C71` and `0x0C7C31–0x0C7CC6`.

## Normalized item power and ordered products

The normalized helper uses the same level exponent, beginning with
the supplied binary32 item level. Its rank exponent multiplies the
converted rank argument by 0.25. It multiplies the narrowed level and
rank power results in that order, saves the product and divides by
the narrowed third power of binary64 two with binary64 exponent three.
The helper returns that stored quotient. The 0.1 creature scale and
item coefficients are applied by their respective outer queries.

Each item helper stores the returned binary32 normalized power. It
multiplies that value by the selected coefficient, then multiplies the
rounded result by the completed seed/material factor. Every admitted
item takes all three power calls, including when its factor is zero.
Client return stores are `0x0C6C8C` and `0x0C7CE1`. Server return
stores are `0x013BAC` and `0x014431`.

## Small-result comparison and exceptional values

Each item helper compares binary32 0.001 as the first comparison
operand and the completed candidate as the second. If the threshold
is ordered and strictly greater, the helper returns positive zero.
Equal or higher ordered values survive. Finite negative values,
negative infinity and either signed zero take the positive-zero path.
Positive infinity survives. An unordered comparison also takes the
candidate return path if native execution completes the comparison.
Return/conversion payload behavior and unmasked exceptions retain the
ambient native controls. The threshold is applied per item before any
creature sum. Evidence includes client `0x0C6C99–0x0C6CBD` and
`0x0C7CEE–0x0C7D12`, and server `0x013BB9–0x013BDD` and
`0x01443E–0x014462`.

## Critical decision and bonuses

The critical reader walks the creature's resident status list at
`+0x1178` and tests the type byte at node `+0x08`. Finding type 11
sets the threshold to binary32 one, independent of the entry's duration
or strength. That route goes straight to the draw. With no type 11,
read the creature critical contribution and store it as binary32. Form
binary32 `+0x1190` times 0.15, then add that bonus to the stat with the
stat as first operand. The resulting threshold is used as formed.

Both routes call the original `rand` once. Convert the signed 32-bit
result to binary32 and divide by binary32 32,767. The critical result
is true only for an ordered threshold strictly greater than the rounded
quotient. In particular threshold one and draw 32,767 give false. The
draw occurs at client `0x0444E5` and server `0x00F565`, through import
slots `0x2FC468` and `0x1582F4`, respectively. The threshold and return
paths are client `0x0444A7–0x04451E` and server `0x00F527–0x00F59E`.

The ordinary common candidate path checks actor byte `+0x13C0` first.
A nonzero byte forces critical and bypasses the critical reader and its
draw. A clear byte calls the reader at client `0x2144BF`, server
`0x13A27F`. For the alternate 32/33 heal the amount is already formed
when the reader is called at `0x214AEC` or `0x13A8AC`. That call ignores
the latch and writes only the critical flag for the heal, without a
critical multiplier on its amount.

## Total attack speed and haste bonus gates

The speed reader starts with binary32 one for control-kind byte
`+0x60` equal to zero. For another kind, convert unsigned rank byte
`+0x1A8` to binary32, multiply by 0.0625 and add 0.75 to that product.
Class byte `+0x140` equal to one with specialization byte `+0x141`
equal to zero adds half of the combo fraction. Class 3 specialization
1 adds the full fraction. Other combinations add neither bonus.

An applicable combo branch first calls the cap reader, then reads
signed combo at `+0x70`. Convert the cap and combo to binary32 and
divide combo by cap. A first ordered fraction strictly above one is
replaced with one. The other path calls the cap reader again, reads
combo again and recomputes the division. This second value is used
as formed, including negative values. Multiply by one half for the
class-1 bonus, and add the applicable fraction term as the first
operand to the current speed. Evidence is client `0x047716–0x047804`
and server `0x012166–0x012254`.

The cap helper multiplies binary32 creature `+0x17C` by the selected
integer-valued binary32 scale, then truncates the rounded product
toward zero to signed 32-bit. With category 3 at `+0xAA8`, subtype 6
uses 60, subtype 8 uses 80, subtypes 11, 12, 15, 16 and 17 use 20,
and other subtypes use 50. For another category, mask `0x10` in
creature `+0x7E` selects 20 and its clear case selects 50. The native
cap has no positive minimum. Evidence is client `0x044CB0–0x044D28`
with table `0x044D2C`, and server `0x00FCF0–0x00FD68` with table
`0x00FD6C`.

After combo processing the speed reader always calls the creature
haste contribution reader and adds its stored return as the first
operand to the current speed. Then it searches the resident list at
`+0x1178` for type 12 at node `+0x08`, using type presence alone.
With no match, return the accumulated speed. With a match, read signed
32-bit skill at `+0x1158`. When that value is zero and control kind is
nonzero, use signed level divided by two toward zero, plus the unsigned
rank byte and one, with 32-bit integer arithmetic. Other cases retain
the stored skill value.

Convert effective skill to binary32, multiply by 0.1, then add one.
Divide one by that sum, subtract the quotient from one, then add one
to the difference. Multiply that completed factor by the accumulated
speed, factor first, and return the stored binary32 result. The skill
reads come from the creature rather than the status strength or time.
Evidence spans client `0x047809–0x0478AA` and server
`0x012259–0x0122FA`.

## Duration and pulse sampling

| Timing reader | Client RVA | Server RVA |
| --- | --- | --- |
| Anticipation | `0x03CAA0` | `0x007DB0` |
| Middle | `0x047310` | `0x011D60` |
| Recovery | `0x044270` | `0x00F2F0` |
| Total | `0x03D1A0` | `0x0084B0` |

The three single-leg readers accept an explicit signed action. A
negative argument selects the unsigned live byte at creature `+0x68`.
Their lookup bounds are unsigned 105 for anticipation and 104 for
middle and recovery. Their tables select fixed integer cases and scaled
cases. A scaled case reads the speed once, stores it as binary32,
multiplies by creature binary32 `+0x17C`, and divides its binary32
numerator by the rounded product. It truncates the quotient toward
zero to signed 32-bit milliseconds. A fixed case returns its integer
without a speed read.

The total reader re-reads the live action separately for anticipation,
middle and recovery, in that order. Each reached scaled leg samples
speed itself. The three signed integer results are added after each
leg's conversion. The final integer additions use 32-bit arithmetic.
The two images' byte ordinals and relative pointer destinations agree
for all four readers, including the total's three tables.

For actions 30 and 32 the anticipation numerator is 800 and the middle
is fixed at 600 ms. For actions 31 and 33 the anticipation numerator is
1,600 and the middle is fixed at 1,200 ms. The recovery numerator is 100
for all four. Thus the total's anticipation and recovery sample speed
separately, and its middle consumes no speed sample for these actions.
Evidence includes client `0x03CBD6–0x03CC09`, `0x04743F`, `0x047507`,
`0x03D2D2–0x03D301`, `0x03D644`, `0x03D718` and `0x03D949`. The
corresponding server destinations start at `0x007EE6`, `0x007F00`,
`0x011E8F`, `0x011F57`, `0x0085E2`, `0x0085FC`, `0x008954`,
`0x008A28` and `0x008C59`.

The common pulse selector admits actions 5, 11, 30–33, 72, 86 and 91.
The period branch is entered with live signed clock at least the
sampled anticipation and strictly below a separately evaluated
anticipation plus middle. Actions 5 and 11 use a fresh middle result
divided by three. Actions 30 and 32 use a fresh middle divided by six,
and actions 31 and 33 divide it by twelve. The divisions truncate
toward zero. With their fixed middle values, actions 30–33 all obtain
100 ms periods. Actions 72 and 86 use 250 ms, and action 91 uses
1,000 ms. The gated-out path sets both saved bounds to the anticipation
read. Evidence is client `0x212CB2–0x212DDD`, `0x213037–0x213052`
and server `0x138A72–0x138B9D`, `0x138DF7–0x138E12`.

A later common retiming gate requires byte `+0x13B8` clear, the clock
projected by the frame step at least anticipation plus middle, live
clock strictly below total duration and roll `+0x128` equal to zero.
On admission it reads speed, clears signed combo `+0x70`, then reads
speed again. Divide the old binary32 result by the new binary32 result,
multiply by the live clock converted to binary32, and truncate toward
zero to the stored signed clock. The saved window bounds survive this
clock write. Evidence is client `0x213448–0x2134EE` and server
`0x139208–0x1392AE`.

## Original-image re-certification (2026-11-17)

The previously documented client stat numerics, raw item boundaries
and sequential slot additions all matched the original image. The
re-certification identifies the original server counterparts and extends
the earlier stat-only scope to the critical decision, total speed and
these timing gates. The two instruction exports were checked against
the PE section bytes. Corresponding instructions, pointed literal bits,
call targets and table entries were checked independently in the two
images. The four duration readers, eight stat and decision readers, and
the named common-action sites have matched arithmetic and conditions.
This establishes those bounded client and server reads.

The game-owned stat and speed readers leave the creature and item
records and game RNG state unchanged. Critical draws are at the
identified decision and caller boundaries. Runtime power implementation,
last-bit behavior outside the game-owned rounding stages, unusual
floating-point controls, zero divisors, exceptional arithmetic, other
world callers, full per-action schedules and runtime acceptance remain
open.
