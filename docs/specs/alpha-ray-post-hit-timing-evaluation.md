# Alpha ray timing after hit sampling

**FINAL / STABLE / HELD**

This supplement specifies the timing observations used by the secondary
projectile gates, action-94 sound 40, and the action-28/95 sound grid.
It clarifies `alpha-combat-beam-leftovers.md` section 8 and
`alpha-strike-call-sites.md` B7 using the original x86 images.
All addresses are RVAs.

| Image | SHA-256 |
| --- | --- |
| C, `Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| S, `Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

The actor's current stored age is the signed 32-bit field at `+0x6c`.
The piece length remains the original 32-bit argument for this world step.
The threshold called B here is the current result of the native helper with
argument -1, C `0x03caa0` or S `0x007db0`.

Each in-loop secondary projectile gate first checks class byte `+0x140`
equal to 3 and specialization byte `+0x141` equal to 1. It then performs
these observations in order.

1. Call the threshold helper.
2. Reload the current actor pointer and compare its stored age against that
   returned threshold, requiring signed age less than or equal to B.
3. Call the threshold helper again.
4. Reload the actor pointer and its current stored age, add the original
   piece length in 32 bits, and require that signed sum to exceed the new B.

These observations occur separately at each reached gate after the candidate's
hit-resolver call. The dedicated helper calls are S `0x13c5a9` and
`0x13c5bf`, with age observations at S `0x13c5b4` and
`0x13c5ca`. The client counterparts are C `0x2167e9`,
`0x2167ff`, `0x2167f4` and `0x21680a`. The piece additions are
S `0x13c5cd` and C `0x21680d`. The post-loop combo update follows
these secondary gates.

After all ray samples, the action's nonzero local hit flag triggers an
increment of actor `+0x70`, a native helper call, and a zero write to
`+0x74` (S `0x13c771` through `0x13c78a`, C `0x2169b1`
through `0x2169ca`). The following empty-ray branch tests whether the
source ID collection at `+0x11ac` is empty. It applies to actions 28
and 95 when that collection is empty. Its action-byte exclusion is 104.

After its effect record append, the empty-ray branch evaluates the native
rate helper, C `0x047700` or S `0x012150`, and stores the result
as binary32. It then clears actor `+0x70`, calls that helper again and
stores its second result as binary32. It divides the first stored rate by
the second in binary32. Only then does it read current signed age, convert
that integer to binary32, multiply the rate ratio by the converted age in
binary32, truncate toward zero to signed 32 bits, and store the result back
to actor `+0x6c` (S `0x13c895` through `0x13c8db`,
C `0x216ad5` through `0x216b1b`).

Action 94, byte `0x5e`, then reaches the sound-40 gate. This gate
evaluates the threshold helper first and compares the actor's current stored
age against that result, requiring signed age less than or equal to B.
It next calls the helper again, reloads current age, adds the original piece
length in 32 bits, and requires the signed sum to exceed that fresh B.
The sound record is emitted only after these comparisons pass.

The dedicated observations are at S `0x13c8f4`, `0x13c8f9`,
`0x13c906` and `0x13c90b`, followed by the addition at
S `0x13c90e`. The corresponding client observations are at
C `0x216b34`, `0x216b39`, `0x216b46` and `0x216b4b`,
followed by the addition at C `0x216b4e`. These reads occur after the
combo and applicable empty-ray updates. The resulting sound identifier is
40 (S `0x13c95d`, C `0x216b9d`).

Actions 28 and 95 reach the later sound-grid check using current stored age,
including the empty-ray rescale when it ran. The check reads that age once.
First it forms the 32-bit sum of the read age and the original piece length,
then divides the signed sum by 200 with truncation toward zero. It next
divides the same read age by 200 with that signed rule and compares the
two quotients. Different quotients open the sound emission branch
(S `0x13ca3a` through
`0x13ca74`, C `0x216c7a` through `0x216cb4`).
The sound identifier is 36 when the action's local hit flag is zero and
37 when it is nonzero (S `0x13caa7` through `0x13cad8`,
C `0x216ce7` through `0x216d18`).

Complete rate formulas, nonfinite conversion behavior and runtime timing
remain open.
