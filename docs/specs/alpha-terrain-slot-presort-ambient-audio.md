# Alpha ambient audio during terrain-slot traversal

FINAL / STABLE / HELD. Original-client analysis, 2026-11-21.

## Scope and evidence

The owner's original 32-bit Alpha Cube.exe has SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
RVAs below use image base 0x400000. The render pass at 0x0AC260 was examined
through the original instruction listing and its executable bytes. Additional
evidence includes the fixed-coordinate arithmetic and positional sound
dispatcher at 0x084350. This record covers the pre-sort effects of the strict
comparison with 4096 and the dispatcher's own distance cutoff.

## Record order and retained distance

Before sorting terrain slots for drawing, the renderer walks the active grid
with X as the outer loop and Y as the inner loop. Each runs from the client
origin (+0x2AC for X, +0x2B0 for Y) up to but excluding origin plus the grid
width at +0x2DC. It skips coordinates outside 0 through 524287. Within that
range it addresses the physical slot by each axis modulo width, X varying
fastest in physical storage. See 0x0AC4DA through 0x0AC58A and 0x0AD42D
through 0x0AD472.

Under the slot's critical section (+0x250), its key at +0x6C is refreshed
only when its active solid count at +0x0C is nonzero, the two slot coordinates
at +0x18 match the current cell and its box passes the range/visibility query
at 0x07F3C0. This query receives the bounds at +0x20 and +0x38 and the
binary32 range built by adding 22.4 and then 32 to client+0x1D4.
The qualifying slot enters the draw candidate collection. The key is the
squared distance between the fixed XYZ at slot+0x50 and client+0x140. The
native dot product multiplies each component difference by itself in 64 bits,
divides each product by 65536 with signed truncation toward zero, sums the
three results, converts the signed sum to binary32 and multiplies by 1/65536.
The key write is at 0x0AC737.

Any failed refresh condition reaches the decoration traversal with the
previous contents of +0x6C intact. In particular a slot can fail draw
admission and still process its current decoration list against a retained
key. There is no key reset on these failure branches. The list at +0x240
is visited forward from its head link through its sentinel. For each node,
the decoration payload begins at node+0x08. Its first four bytes are the
kind and its signed 64-bit fixed XYZ starts at payload+0x08.

For each decoration, the separate flag-dependent light-candidate operation
runs first. Irrespective of whether that operation admitted a light, the
audio path at 0x0ACAB3 through 0x0ACABF requires an ordered slot key strictly
below binary32 4096. Equality, greater values and unordered values skip all
time checks and random draws here. For freshly calculated ordinary distances
the threshold corresponds to a radius of 64 world units around the slot
reference. This comparison never measures the individual sound source.

## Time of day, kinds and sound selection

The day clock is the signed 32-bit value at client+0x800440. The following
table gives the complete eligible combinations. Interval endpoints matter.
The hour labels assume the native day scale of 86400000 units. Base gain
is the value passed to the sound dispatcher before distance attenuation.

| Decoration kind | Day-clock condition | Period | Random gate | Sound ID and name | Base gain | Pitch base |
| --- | --- | ---: | ---: | --- | ---: | ---: |
| 61 | Strictly between 21600000 and 75600000 (06:00 to 21:00) | 1000 | modulo 10 | 94, bird1.wav | 0.1 | 0.9 |
| 62 | Same open interval | 1000 | modulo 10 | 95, bird2.wav | 0.1 | 0.9 |
| 63 | Same open interval | 1000 | modulo 10 | 96, bird3.wav | 0.1 | 0.9 |
| 61 | At or below 21600000, or at or above 75600000 | 1000 | modulo 10 | 99, owl1.wav or 100, owl2.wav | 0.1 | 0.5 |
| 2, 3, 4 | Strictly between 43200000 and 86400000 (12:00 to 24:00) | 5000 | modulo 70 | 97, cricket1.wav | 0.05 | 0.9 |
| 0, 1 | Same open interval | 5000 | modulo 70 | 98, cricket2.wav | 0.05 | 0.9 |

Kinds 62 and 63 have no owl branch. The 12:00 boundary excludes crickets
and the exact 21:00 boundary already selects the owl for kind 61. The code
compares the supplied signed clock directly. This branch consumes no random
draws for all other kinds, including kind 60. Kind 60's separate particle
opportunity occurs later during the sorted slot traversal at 0x0B1EB4.

## Spatial phase and random-draw order

For the selected period, first obtain the source's whole-unit X from fixed X
divided by 65536. For Y, add 333 times 65536 to fixed Y before dividing by
65536. Both divisions are signed, truncate toward zero and supply their low
32-bit results to the phase arithmetic. The position offset affects timing
only. Positional sound receives the original, unshifted XYZ.

Take the signed remainder of each resulting coordinate on division by the
period. Add the two remainders to the signed clock at client+0x8003A0.
Divide this sum by the period with truncation toward zero to obtain the
current time bucket. For the second bucket also include the signed frame
increment at client+0x8006E8 in the sum before dividing. These additions,
remainders and final divisions use 32-bit arithmetic. A negative remainder
keeps its sign. The clock and increment are separate from the day clock.

Identical buckets end the test. Different buckets create one opportunity
for the current decoration, even if a single increment crosses several
boundaries. No per-decoration last-bucket field is read or written here.
For every opportunity the original CRT random stream advances as follows.

1. Draw once and take the signed remainder modulo the table's gate divisor.
   A nonzero remainder ends the opportunity.
2. On a zero remainder, draw again for pitch. Convert the draw to binary32,
   multiply by binary32 0.2, divide by 32767 and add the table's binary32
   pitch base. The nominal pitch range is 0.9 through 1.1 for birds and
   crickets and 0.5 through 0.7 for owls.
3. Only the owl branch draws a third time. Its remainder modulo 2 selects
   99 for an even draw and 100 for an odd draw. The pitch draw precedes
   the sound variant draw.

The three bird branches start at 0x0ACAE3, 0x0ACC62 and 0x0ACDDE. The
owl branch starts at 0x0ACF5A and the cricket dispatch at 0x0AD0ED. They
join at 0x0AD406. All draws above take place before positional audibility
is checked. The next decoration is visited at 0x0AD411, so failed random
gates and inaudible requests still affect subsequent stream positions when
they have consumed a draw.

## Independent positional cutoff

RVA 0x084350 receives the chosen ID, unmodified record position, base gain
and pitch. For each axis it subtracts the client reference at +0x140 from
the source in fixed signed 64-bit coordinates, narrows to binary32 and
multiplies by 1/65536. It sums the squares as Y squared plus X squared,
then Z squared. A result strictly above 10000 returns before resolving
the sample or requesting playback.

For a passing distance, take a binary64 square root of the squared distance
and narrow it to binary32. The gain multiplier is 1 minus this length
divided by 100. The dispatcher applies that multiplier to the base gain,
then multiplies by the signed client sound-volume setting at +0x184 and
by binary32 0.01. It looks up the named WAV through client+0x800714 and
passes the resulting handle, gain, signed stereo pan clamped to [-1, 1],
the requested pitch and a final zero to the playback service. Equality at
100 world units reaches this service with zero gain. This 100-unit check
is independent of the earlier slot-key cutoff at 4096.

## Limits

This record establishes the client-side decisions and arguments for the
specified image. It does not assign an initial value to an unobserved
slot key, model all clock producers, or establish audible output from the
playback backend. It covers the pre-sort decoration audio branch, independent
of later immediate or deferred rendering. No runtime capture was used.
