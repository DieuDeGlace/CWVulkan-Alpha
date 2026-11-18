# Alpha projectile terrain ray and variant 2 contact

Status: FINAL / STABLE / HELD

## Scope and evidence

These rulings supplement `alpha-connected-projectile-receipt-step.md`
§§4–5. All addresses are RVAs in the original 32-bit Cube.exe,
3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
The connected client uses the same terrain query for local and
nonlocal projectiles. The covered projectile types are 0 through 4.

## 1. Post-advance projectile query

The projectile moves by its precomputed substep displacement in
fixed units, then tests the cell at the advanced position. The ray
call is on the passable-cell route. It uses the advanced position
as the origin, in three signed 64-bit components at 65,536 units
per block, and the current three-component binary32 velocity.
Its distance limit is binary32 velocity length times the seconds
of the full world step. The ray does not divide that limit by the
projectile substep count. Form the binary32 component squares,
add X and Y first, then Z, and take the square root in binary64.
Round the result to binary32.

The direction helper divides binary32 one by that length and
multiplies each original component by the quotient. A second helper
multiplies all three results by binary32 minus one. The ray moves
along the direction supplied. With a finite positive computed
length, the query is backward from the advanced origin. The first
flag is zero and chooses continuation through passable cells. The
second flag is one and enables the placed-object pass. The caller
rolls back the completed substep using the binary32 result and
sets terrain contact only if its limit is ordered strictly greater
than the returned distance. Equality and unordered comparisons
leave the advanced position in place. This ray-return branch does
not emit the type-0 solid-cell effect or draw a random number.

The call boundary is `0x22223E–0x2222D3`, with length helper
`0x023F20`, normalization `0x027870`, sign reversal `0x068E20`
and ray `0x1A35D0`. The earlier solid-cell point test and impact
effect are `0x221FCD–0x222239`.

## 2. Terrain distance

### 2.1 Cell coordinates and continuation

Initialize the current distance to positive zero. For each origin
component, divide the fixed value by 65,536 as signed integers,
truncating toward zero, then subtract one for a negative original
component. This subtracts one even at an exact negative block
boundary. Keep those three integer cell coordinates for the walk.
For a positive ordered limit, read the initial cell before crossing
any boundary (`0x1A35E6–0x1A36A0`).

The horizontal cell coordinates must each be nonnegative and
strictly below 16,777,216. Divide X and Y by 256 for the chunk
lookup `0x034D10`. The column is a 32-byte record from chunk
`+0xA8` at index 256 times the Y remainder plus the X remainder.
The signed base Z is at column `+0x10` and the signed cell count at
`+0x1C`. The in-range cell pointer is column `+0x18` plus four times
the Z index relative to the base, through `0x02F730`.

An out-of-range X or Y, missing chunk, zero resulting column
address or Z below the base yields the fixed cell whose type byte
is one. At or above base plus count, the fallback is type byte zero
for Z strictly above zero and `0x82` otherwise. An in-range cell
with low-five-bit type zero at Z at most zero also uses the `0x82`
fallback if its type byte lacks mask `0x40`. The startup writes for
these three fallback records are `0x2FAB00`, `0x2FAAD0` and
`0x2FAB30` respectively. The walk takes the low five bits of the
selected cell byte at `+0x03`. Values 0 and 2 are passable. With
the projectile's zero flag, any other value ends terrain traversal
at the current distance (`0x1A36A0–0x1A3796`).

### 2.2 Fixed boundary step

After a passable cell, reconstruct the current fixed ray point
from the initial origin and accumulated distance. For each
component multiply direction by distance in binary32, multiply by
binary32 65,536, truncate toward zero to signed 64-bit and add to
the original fixed component. This reconstruction happens at every
advance (`0x1A37A7–0x1A3891`).

Start the next increment at binary32 10 with X as the selected
axis. Visit X, Y and Z in that order. Skip an axis if its
binary32 squared direction is ordered strictly below the binary32
constant nearest to 0.000001. The original constant is at
`0x31DAF4`. For a positive direction, use the upper plane of the
current cell, the cell coordinate plus one multiplied by 65,536.
For a nonpositive direction, use the lower plane at the cell
coordinate multiplied by 65,536.

Subtract the reconstructed point component from that plane in
signed fixed units. Convert that signed difference to binary32,
divide by the direction component in binary32, and truncate the
quotient toward zero to signed 64-bit. Convert it back to binary32
and multiply by exactly 1/65,536. Use this as the candidate distance
increment. Replace the selected axis and increment only when the
candidate is ordered strictly smaller. Thus equal face increments
keep the first selected axis. There is no separate positive-distance
test, and zero-distance boundary crossings are possible. If no
candidate displaces the starting pair, the advance keeps X and ten
(`0x1A3891–0x1A3A16`).

Increment only the selected cell coordinate for a positive
component, otherwise decrement it. Add the chosen increment to
the distance in binary32. Continue with the next cell only if
the maximum is ordered strictly greater than the new distance.
A step reaching the limit can update the cell coordinate without
probing that cell. The iteration counter starts at zero. The cell
test runs before the signed count test, and a count strictly
above 500 ends traversal, allowing at most 501 boundary advances.
At the join, clamp the distance to the limit if it is ordered
strictly greater than the limit
(`0x1A3796–0x1A37A7`, `0x1A3A1C–0x1A3A93`).

### 2.3 Zero, boundary and invalid values

With a positive limit, an initially blocking cell leaves the
terrain distance at zero. Walking out of horizontal coverage or
into a missing chunk stops at the current distance through the same
blocking-cell fallback. For an origin exactly on a nonnegative
block plane and a negative direction on that axis, the lower-plane
increment can be zero. If chosen, the adjacent lower cell may
therefore block at the same distance as the origin.

A nonpositive or unordered limit skips the terrain loop. Ordered
negative limits are then returned unchanged after the clamp. A
zero or NaN limit leaves the initial positive zero. The optional
object pass cannot lower a nonpositive best distance since it
requires a nonnegative candidate strictly below the current best.
The projectile's zero-length velocity still reaches normalization
without a zero check. For finite step seconds its limit is zero,
so it receives distance zero and the strict caller comparison does
not flag ray contact. A NaN limit likewise cannot pass the caller's
ordered comparison.

There is no general invalid-direction rejection. In particular, an
unordered squared component reaches the boundary calculation
because the skip comparison requires an ordered result. With
masked x87 invalid conversion, a NaN boundary quotient converts to
the signed 64-bit indefinite value at `0x28D946–0x28D9BA`. The
subsequent scale can make that a negative increment of
minus 140,737,488,355,328. These conditions do not establish any
single miss value for arbitrary nonfinite input. The zero and
unordered-limit rules above apply regardless of that separate
positive-limit case (`0x1A3681–0x1A3696`, `0x1A38B0–0x1A3A93`).

## 3. Placed-object distance

For the projectile's nonzero second flag, the routine starts the
object pass with the terrain result as its current best. It forms
two chunk endpoints. The origin endpoint uses each original
horizontal component divided by 65,536 with signed truncation
and no negative correction. The other endpoint uses the final
terrain cell X and Y, including a step that reached or exceeded
the limit. It divides the resulting block coordinates by 256
with signed truncation to obtain chunk coordinates.

Visit the inclusive rectangle between the minima and maxima
of those endpoints. Traverse ascending X on the outer loop and
ascending Y on the inner loop. Skip chunks with a negative
coordinate, a coordinate at least 65,536, or missing group or
chunk pointers. The lookup uses groups of 64 by 64
chunks from the world pointer table at `+0xBC`, with the X index
before Y. At each chunk, visit the vector from chunk `+0x0C`
through its end pointer at `+0x10`, in stored order. Each object
record is `0x188` bytes
(`0x1A3A93–0x1A3C63`, `0x1A469C–0x1A46F2`).

The accepted 32-bit object types at `+0x00` are 3, 4, 5 and 8.
The byte at `+0x30` must be nonzero. The fixed position at
`+0x08` is the horizontal center and the bottom Z. The three
binary32 sizes are at `+0x24`, `+0x28` and `+0x2C`. An odd
signed 32-bit orientation at `+0x20` swaps the horizontal sizes.
The fixed horizontal half-width uses the binary32 size times 0.5
then times 65,536, converted toward zero to signed 64-bit. The
fixed height uses the binary32 Z size times 65,536 with the same
conversion (`0x1A3C63–0x1A3CE9`).

Test the lower then upper X side and then the lower and upper
Y side. An exactly zero direction component skips its two sides.
There are no Z faces. For a tested face, subtract the original
axis coordinate from the fixed face coordinate. Convert the
difference to binary32, multiply by 1/65,536 and then divide
by the signed direction component. Require an ordered
nonnegative result that is strictly below the current best.
This branch keeps the binary32 distance and has no integer
truncation of the quotient.

For an admitted distance, reconstruct the fixed point from
the original origin and direction at that distance by the same
component conversion as the terrain walk. For an X side, require
Y in the half-open horizontal bounds and Z in the half-open
vertical bounds. For a Y side, test X and Z analogously. Lower
bounds are inclusive and upper bounds exclusive. A passing
candidate replaces the best distance. Equal distances retain
the previous result, including a terrain result at the same
distance. This pass skips the object entirely when both
horizontal components are zero, even if the ray passes its top
or bottom (`0x1A3CE9–0x1A469C`). The final best is returned at
`0x1A46F2–0x1A4708`. Neither pass calls the random generator.

## 4. Direct variant 2 relationship

For direct contact with variant byte `+0x64` equal to two, the
record's resolved source and live target are passed in that
order to `0x196CA0`. The append requires that a source exists,
the relation returns false, and the target's controller byte at
`+0x60` differs from six. The relation itself tests in this order.

1. Source kind five with target kind zero returns false
   immediately. Either kind six then returns false.
2. Exactly one kind one returns true. If both are kind one,
   compare their classifier results at `0x044680`. Unequal
   results return true. The native equal-result route repeats
   the pair of classifier calls once before the status tests.
3. Test low-byte mask `0x20` at source `+0x124` and then at
   target `+0x124`. Either set returns true. Otherwise return
   false.

The classifier first returns false for any set bit of mask
`0x1A00` in the 16-bit field at `+0x7E`. Otherwise it returns
true for species at 32-bit `+0x64` in the following set, and
false for all other values.

19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55,
56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107,
145, 146, 147.

The classifier and the relation make no random draw, and the
relation has no pointer-equality or key check of its own
(`0x044680–0x044759`, `0x196CA0–0x196D2A`).

The earlier direct candidate gate `0x1988A0` uses the same
kind-one and status logic, but rejects identical pointers,
returns true if either kind is six, and has no source-kind-five
exception. Type 3 variant 2 bypasses that earlier source gate
and the linked-ID exclusion. Its direct append can therefore
pass for identical source and target, subject to the later
relation, target kind and the normal contact, roll and cadence
gates. With other covered types using variant 2, a candidate
that passes both relation gates and the target-kind-six rejection
has either a kind-six source, or a kind-five source with a
kind-zero target and status mask `0x20` set on at least one.
The earlier source link and other candidate gates still apply
(`0x22160C–0x221653`, `0x1988A0–0x19892C`,
`0x221D03–0x221E22`).

On an accepted append, retain the projectile source key and
target live key, target position and computed critical byte.
Negate the prepared amount and multiply by binary32 0.5 for
a kind-one source. The record's impulse, stun and kind are zero.
Connected direct application still requires both resolved
pointers equal to the local creature, so a nonlocal source goes
past that application. A rejection here can still reach the
later signed target-clock comparison. The second, impact-area
candidate pass uses the ordinary helper at `0x222AB8` even
for variant 2
(`0x059A30–0x059A8D`, `0x221D39–0x221EBB`).

## 5. Sound 19 reachability

The block beginning at `0x2223EC` would draw at `0x222406`,
append sound 19 at the then-current projectile position with
volume one and pitch binary32 one plus draw times 0.4 divided
by 32,767, and queue the projectile for removal. Its only decoded
incoming branch is the inequality test at `0x22237C`.

The stock turn reaches that test only on the equal-to-two
projectile-type route at `0x222345–0x222349`, followed by the
terrain-contact test. Between the first and second reads of
record `+0x60`, the optional success path changes the resolved
source's combo and timer fields and calls `0x0453B0`. That
helper only traverses the actor's tree and returns. None of
this intervening work writes the projectile type. The world
turn is protected by the same lock as projectile receipt. The
second type read is therefore still two. It takes the
zero-velocity route, and sound 19 and its draw are unreachable
in this stock resident step. Types other than two take the
separate impact branch before the second read
(`0x222345–0x222457`, `0x0453B0–0x04540B`).

## Evidence limits

The ray ruling covers the original client routine with the
projectile flags and representable world coordinates. It does
not assign results for arbitrary arithmetic overflow or for
invalid object storage. The dedicated server's ray copy and
the source of every admitted object record are outside scope.
