# Alpha dedicated action projectile launches

**FINAL / STABLE / HELD**

The primary source is the original x86 `Server.exe`, SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
The comparison source is the original x86 `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
All addresses are RVAs. S identifies the dedicated image and C the client.
Scope begins with an actor reaching the native action dispatcher and follows
the two S launch sites `0x13bf84` and `0x13c6be`.

## Action family and source admission

The dedicated action dispatcher maps action bytes `0x1c`, `0x5e` and
`0x5f` at actor `+0x68` to S `0x13ba28`. The byte remap at
S `0x148a8c` selects entry 4 of the target table at S `0x148a44`
for each of these actions (dispatcher S `0x137bc4` through `0x137bdb`).
The source role is the single byte at actor `+0x60`.

When world byte `+0xb4` is zero, a source with a nonzero role passes the
family's source gate. For role zero, the source pointer must equal the local
pointer at world `+0xb8`. When the world byte is nonzero, every source
must equal that local pointer (S `0x13ba28` through `0x13ba43`).
An absent local pointer therefore rejects a role-zero source at this gate.

The family performs resource work before its timing window. Action `0x1c`
subtracts the binary32 product of the integer piece length converted to
binary32 and constant `0x3951b717` from actor `+0x170`. This constant
is approximately 0.0002. For actions `0x5e` and `0x5f`, a threshold
crossing subtracts the result of S `0x00fb20`. This helper returns zero
for `0x5e`. For `0x5f`, it returns binary32 0.3 (`0x3e99999a`)
when its status-list search finds no type-9 entry and zero when it finds one.
The subsequent native type-9 handling runs when such an entry is present.
If the resulting resource at `+0x170` is strictly negative, the family
stores zero there and exits this action work (S `0x13bb21` through
`0x13bc80`, S `0x00fb20` through `0x00fbb8`).

## Signed age and launch gates

The stored action age is the signed 32-bit field at actor `+0x6c`.
The piece length is a 32-bit integer. Age-plus-piece calculations use a
32-bit addition and signed comparisons. The ordinary age addition in this
world pass occurs later at S `0x13e301`.

The native threshold helper is S `0x007db0` with argument -1. For this
family its return values use these operations.

- Action `0x1c` returns zero.
- Action `0x5e` divides binary32 500.0 by the binary32 product of the
  value returned by S `0x012150` and actor `+0x17c`, then truncates
  the quotient toward zero to a signed 32-bit integer.
- Action `0x5f` performs the same operations with binary32 1,000.0.

The helper rounds the returned rate to binary32 before the multiplication
and division (S `0x007db0` through `0x00812f`, byte remap
S `0x0081c0`, target table S `0x008134`).

The common window requires age plus piece to be at least the threshold
and the stored age to be less than threshold plus window length. Its
window helper is S `0x011d60` with argument -1. For action `0x1c`,
that length is binary32 300.0 divided by the same native rate product and
truncated toward zero. Action `0x5e` uses 500.0 in that calculation.
Action `0x5f` returns the integer 2,000. The gate evaluates threshold for
the age-plus-piece comparison, then evaluates threshold again and obtains
the length for the stored-age bound (S `0x13ba9d` through `0x13bae2`,
S `0x011d60` through `0x011ff3`).

At both launch sites the source must have class byte `+0x140` equal to
3 and specialization byte `+0x141` equal to 1. The stored age must be
less than or equal to the threshold. The 32-bit sum of the stored age and
piece length must be strictly greater than the threshold. Each site calls
the threshold helper for each comparison, in that order. These checks occur
at S `0x13bdfa` through `0x13be44` and S `0x13c58d` through
`0x13c5d5`.

For actions `0x1c` and `0x5f`, later action-hit samples can occur when
the signed quotients of stored-age-minus-threshold and
age-plus-piece-minus-threshold by 200 differ. Division truncates toward
zero. The new age must exceed the threshold and the common window must
still pass. Those later samples retain the launch sites' stricter crossing
checks (S `0x13bc8b` through `0x13bd16`).

## Appends and the ordered actor-hit stage

Before the first launch gate, the family prepares an origin using
S `0x011800`, an aim direction from actor `+0x160` and a binary32
reach from its forward query at S `0x0d6730` with a 200-block argument.
It clears the source collections at `+0x11ac` and `+0x11b4`
(S `0x13bd1c` through `0x13bded`).

When the first gate passes, it constructs one projectile and appends a copy
to the tail of world `+0x14` at S `0x13bf84`, then appends a second
copy to the output collector's `+0x20` at S `0x13bf99`. Its center
is the prepared origin plus the normalized aim direction scaled by reach.
Each scaled binary32 component is converted to signed 64-bit fixed units
by multiplication by 65,536 and truncation toward zero before adding it
to the fixed origin (S `0x13bedb` through `0x13bf2a`,
S `0x002510` through `0x00254d`).

These two appends precede the positive-reach test and the actor-hit stage.
Positive reach starts repeated map scans from the current sample center,
initially the origin. Each scan walks the world `+0x04` actor map in
ascending signed 64-bit actor ID order. It skips null pointers, the source,
the ID at source `+0x11c0`, nonpositive health, IDs already recorded
in source `+0x11ac`, and nonzero actor `+0x128` (S `0x13bf9e`
through `0x13c091`, map search S `0x130600`).

For this family, the horizontal admission uses the native fixed
squared-distance routine S `0x01ce90`. It halves the candidate's X extent,
adds one block, then squares that result in binary32. It multiplies the
square by 65,536 in binary32 and truncates to signed 64-bit fixed units.
The native fixed squared-distance result must be strictly below this bound.
The absolute binary32-converted fixed Z difference must be strictly below
half the candidate's Z extent plus one block. A geometrically admitted ID enters
source `+0x11ac` before the family computes the hit amount and evaluates
the relationship predicate at S `0x0d18c0` (S `0x13c097` through
`0x13c355`, comparison S `0x0dade0`).

An admitted target that passes this predicate reaches the action-hit resolver
at S `0x0cfd50` through the call at S `0x13c488`. That call completes
before the secondary launch gate. A false return still reaches the gate.
A successful return can also enter the separate specialization-2 self-heal
path. The class-3 specialization-1 launch reaches its gate directly after
the resolver (S `0x13c48d` through `0x13c58d`).

When the secondary gate passes, it constructs a projectile at the current
sample center. It appends to world `+0x14` at S `0x13c6be`, then to
collector `+0x20` at S `0x13c6d3`. It then continues the actor scan.
Every qualified candidate can repeat this pair while the native launch
checks still pass (S `0x13c5db` through `0x13c71d`).

After a complete actor-map walk, the family advances the sample center by
one converted normalized aim vector and increments its sample counter.
It continues while reach exceeds that counter. The source hit-ID collection
persists across these samples. After the samples, the source's `+0x70`
counter and related native state can be updated (S `0x13c723` through
`0x13c791`).

## Launch record values

Both sites use the constructor at S `0x022890`, then write the same
payload fields with the center selected above.

- The owner ID at `+0x00` is the source's 64-bit ID. The 64-bit ID
  at `+0x08` retains all-one bits and the 32-bit field at `+0x10`
  is zero.
- The fixed center starts at `+0x18`. The three binary32 values at
  `+0x30`, `+0x34` and `+0x38` are zero. Velocity at
  `+0x3c`, `+0x40` and `+0x44` is zero.
- The amount at `+0x48` is a fresh evaluation of S `0x011ad0`,
  rounded to binary32, then multiplied in binary32 by 0.1
  (`0x3dcccccd`). The half width at `+0x4c` is binary32 4.0.
- Binary32 fields `+0x50` and `+0x54` are 1.0. The 32-bit
  field at `+0x58` and the byte at `+0x5c` are zero.
- The 32-bit kind at `+0x60` is 3 and the effect byte at `+0x64`
  is 2. The signed 32-bit age at `+0x68` and the 32-bit field at
  `+0x6c` are zero.

The paired appends copy the record by value. The list append helper is
S `0x028360` and the payload copy is S `0x0227e0`
(launch initialization S `0x13be50` through `0x13bf99`,
S `0x13c5e1` through `0x13c6d3`).

## Client comparison and evidence limits

The matched client independently has the same family remap and source gate
at C `0x211e04` through `0x211e1b` and C `0x215c68` through
`0x215c83`. Its corresponding class, specialization and age gates are
C `0x21603a` through `0x216084` and C `0x2167cd` through
`0x216815`. Its paired appends are C `0x2161c4` then `0x2161d9`
and C `0x2168fe` then `0x216913`. The checked client fields and
0.1 multiplier agree with the dedicated launch records.

The dispatchers route action `0x68` to S `0x138117` and
C `0x212357`. Reachability of the separate `0x68` comparisons
inside this family's shared native region remains open. The complete
native rate and amount formulas, all upstream actor-dispatch eligibility,
forward-query collision policy, fixed squared-distance overflow behavior,
zero-aim and nonfinite behavior, and live timing remain open.
