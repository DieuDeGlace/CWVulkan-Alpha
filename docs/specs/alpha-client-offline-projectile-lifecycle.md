# Alpha offline client projectile lifecycle

**FINAL / STABLE / HELD**

Source is the original x86 `Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs. This contract covers the standalone client with world byte
`+0xb4` zero and the local creature installed at world `+0xb8`.

## Order within a world step

The client calls the same world routine at `0x20c510` for each 20 ms frame
piece and for the final piece. These calls share the frame's output collections
and pass a null incoming hit collection (`0x08cf20` through `0x08cf49`,
`0x20cd1f` through `0x20cd28`).

The actor pass can create projectiles while resolving actions. The launches at
`0x21546d` and `0x2161c1` append to the tail of the live list at world
`+0x14`. Each also appends a separate launch record to the output list at
`+0x20` (`0x215485`, `0x2161d9`, append helper `0x0861a0`).
These two launches set projectile kind 3, zero age and zero velocity. The
first sets effect 2 for action byte `0x5a` and effect 1 otherwise. The
second sets effect 2 (`0x215373` through `0x215470`, `0x216090`
through `0x2161c4`). The second launch's actor action can also continue into
its own actor hit scan and resolver before the projectile pass
(`0x2161de`, `0x216210`, `0x2166c8`).

The main actor pass ends at `0x21fe61`. The ordinary world object pass walks
world `+0x0c` before the projectile pass begins at `0x220a70`. Projectile
iteration starts at the head of the live world `+0x14` list at `0x220a91`
and proceeds in list order. A launch from the actor pass is therefore already
present for the current piece, and its age and collision sampling begin in that
piece. The separate output copy keeps its launch values.

## Age, movement and actor selection

Projectile records hold the signed 64-bit owner ID at `+0x00`, the fixed
center at `+0x18`, binary32 velocity at `+0x3c`, binary32 collision
half width at `+0x4c`, a flag byte at `+0x5c`, a 32-bit kind at
`+0x60`, an effect byte at `+0x64` and signed 32-bit age in milliseconds
at `+0x68`.

The pass prepares velocity or owner-relative motion according to kind, then
forms displacement from velocity times the piece length in seconds and adds
the piece's integer milliseconds to age (`0x220acd` through `0x220cc1`).
For a projectile that reaches sampling, the count is the truncation of one
plus twice the length of this displacement. It divides the displacement by
the count to obtain each movement increment (`0x221491` through
`0x2214db`). Every sample begins with the actor scan at the current center.
For moving kinds, the projectile's movement and terrain impact tests follow
that scan. Kind 3 skips center movement and the projectile's own terrain
impact tests, so its sample centers can coincide (`0x221f1e`,
`0x221fe8`, `0x2220d1`, `0x222293`, `0x2223c8`).

The target scan uses the world `+0x04` actor map in ascending signed 64-bit
ID order. The local creature and nonlocal actors use the same order
(`0x221590` through `0x221653`, map lookup `0x077d90`, successor
`0x1c3ea0`). Each candidate needs a live pointer and positive health.
If the owner resolves to an actor, the ordinary scan rejects that actor,
requires the relationship test at `0x1988a0` to pass, and rejects the ID
stored at owner `+0x11c0`. A missing owner bypasses those owner filters.
Kind 3 with effect 2 bypasses them as well and has its own effect gate below.

The scan tests the projectile box against the actor's box and checks
visibility between their centers. On each axis the projectile's upper edge
may equal the actor's lower edge, and its lower edge must be strictly below
the actor's upper edge. The contact scan uses the projectile half width at
`+0x4c` and half the actor extent for that axis (`0x221659` through
`0x221939`). A candidate with a nonzero dword at actor `+0x128`
bypasses the ordinary direct hit path. If the owner exists, this branch
records the candidate ID in the owner's collection at `+0x11b4`
(`0x22193f` through `0x22196c`).

## Hit timing and application

For kinds 0, 1 and 4 the direct hit branch opens when the projectile flag is
zero. These kinds end the current body scan at the first admitted body that
reaches the hit decision, including when the flag closes the direct hit
branch. Kind 3 opens its direct hit branch upon crossing a 1,000 ms
age bucket. Kind 2 opens it when the new age exceeds 300 ms and crosses a
100 ms bucket. In both timing checks the old age is the updated age minus
the piece length. A new kind-3 launch is sampled in its launch piece and
reaches its first pulse boundary at age 1,000 ms. Kind 2 and kind 3 can
continue scanning other bodies
(`0x221971` through `0x221a42`, `0x221ec1` through `0x221f17`).

The ordinary direct path calls the hit resolver at `0x196d30` inline
(`0x221e8c`). Its connected-world local-target exclusion is bypassed in
standalone play. In the ordinary accepted result, the resolver appends the
hit record to the output and invokes the application routine `0x195a60`
before returning (`0x196d82` through `0x196dae`, `0x198687`
through `0x1986d7`). Both local and nonlocal target records can change
health immediately. The apply routine resolves the target ID again, requires
positive health and applies its usual defenses and effect handling. Its
positive ordinary health subtraction checks damage eligibility for the
target and any resolved source. Its knockback velocity assignment has a
narrower gate. The target's actor role byte at `+0x60` must differ from 6
and the target must either be the local pointer or have a nonzero role byte
(`0x195aab` through `0x195b3e`, `0x195dbd` through `0x195e36`,
`0x195ed2` through `0x196052`).

Effect 2 has a direct healing branch. It requires a resolved owner, a false
result from the separate relationship test at `0x196ca0`, and a target role
other than 6. It appends a signed hit record with the amount negated, halving
it for owner role 1, then calls `0x195a60` immediately in standalone play.
A negative hit amount adds health up to the target's
maximum (`0x221d03` through `0x221e22`, `0x196008` through
`0x196052`). This path can admit an owner as a candidate when kind 3
bypasses the ordinary owner filter.

On the terminal collision path, kind 1 or a nonzero projectile flag opens a
second actor scan. This scan tests a box of half width 5 blocks around the
current projectile center and uses the same map order. It ordinarily applies
through `0x196d30` again as each candidate qualifies. It stops early in
standalone play if the resolved owner has role byte zero and differs from
the local pointer. The first nonnull candidate reaches that source check
before any terminal hit
(`0x222457` through `0x2225e1`, `0x2225e7` through `0x222879`,
`0x222ab8`). Hit application in the projectile pass can affect later
samples and projectiles in the same piece. The main actor and object passes
have already run, and the post-projectile follow-up starts at `0x220e51`.

## Removal and the next world step

Kind 2 enters retirement when its owner lookup fails or its pre-step age
exceeds 1,200 ms. That check occurs before the age increment. Other kinds
enter retirement when their updated age exceeds 5,000 ms, before sampling
that piece (`0x220ae2` through `0x220b11`, `0x220cbb` through
`0x220d68`). Kind 2 responds to a terrain impact by zeroing its velocity
and continuing the sample loop (`0x222345` through `0x2223b0`).

A terminal collision normally stages the live node for retirement. Kind 1
with effect 2 keeps that node live as kind 3.
It sets age to 3,000 ms when the old flag is zero and to zero otherwise,
and multiplies the hit amount at `+0x48` by binary32 0.05
(`0x3d4ccccd`). For the half width at `+0x4c`, it first multiplies the
old binary32 value at `+0x54` by 5.0, then adds 5.0 to that binary32
product. The operations at `0x222b77` and `0x222b7f` both read
`0x40a00000` at RVA `0x2fce24`. After storing the half width, the
conversion clears `+0x54` and the flag, then advances to the next node.
The converted node resumes sampling on the following world step
(`0x222b35` through `0x222b92`).

The matched x86 `Server.exe` has SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
Its terminal conversion uses the same kind and effect gates, age selection,
field updates and list continuation (`0x1488f5` through `0x148952`).
The dedicated multiply at `0x148937` and addition at `0x14893f` both
read the binary32 5.0 at RVA `0x15872c`.

Retirement stages an iterator at `0x220d68`, then the walk advances to the
next live node. When this pass finishes, it erases the staged nodes from
world `+0x14` in the same piece (`0x220db9` through `0x220e42`,
erase helper `0x077c60`). The next piece starts from the surviving live
nodes and any launches from its own actor pass.

## Evidence limits

Static tracing verifies these branches for the matched image. The complete
action-to-projectile mapping, owner steering formulas, damage and defense
formulas, collision presentation, and live behavior for unusual state remain
open. The dedicated comparison covers terminal conversion. The connected
client path remains open.
