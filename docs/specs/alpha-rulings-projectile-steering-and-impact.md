# Alpha projectile steering and impact amount

Status: FINAL / STABLE / HELD

## Scope and evidence

This ruling supplements `alpha-connected-projectile-receipt-step.md`
§§3–5 and `alpha-rulings-projectile-ray-and-variant.md`. The
original 32-bit Cube.exe is 3,885,568 bytes, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs in that image. The impact ruling follows a
connected nonlocal source through the ordinary hit-call boundary.

## 1. Type 2 source and age

The resident type at record `+0x60` selects type 2 at `0x220AD9`.
First look up the source using the full record key at `+0x00`,
through `0x02F000`. If the returned pointer is null, queue the
projectile for removal and leave its visit. A resolved pointer
continues directly to the signed 32-bit age at record `+0x68`,
without a source-health, controller or local-owner test.

Use the age as it stands on entry, before adding this world
step's milliseconds. Age strictly greater than 1,200 queues
the same removal. Ages at most 800 use outbound steering.
Ages greater than 800 through 1,100 use the return velocity
assignment. Ages greater than 1,100 through 1,200 use the
catch position blend. The source lookup and both removal gates
precede every steering write. This removal path adds no
steering draw and does not retime the source
(`0x220ACD–0x220BB1`, `0x220D68–0x220D75`).

### 1.1 Outbound steering

Read the three binary32 source aim components at `+0x160`,
`+0x164` and `+0x168`. For each component multiply by binary32
65,536 and convert toward zero to signed 64-bit fixed units.
Add this fixed aim vector to the source's live fixed position
at `+0x10`. Subtract the projectile's current fixed position
at record `+0x18`, component by component in 64-bit fixed
arithmetic.

Convert each resulting signed fixed difference to binary32
first, then multiply by exactly 1/65,536 in binary32. Multiply
each converted component by binary32 five to get the desired
velocity. The latter helper at `0x0252F0` has only three scalar
products. The full offset determines the magnitude, including a
zero desired vector for a zero fixed difference. This resolves
the fixed-length wording of the earlier projectile contract.
Neither this branch nor the return velocity construction below
normalizes its offset
(`0x220BF6–0x220C65`, `0x02C460–0x02C4FD`,
`0x02C7A0–0x02C854`, `0x0252F0–0x025324`).

Pass that desired vector once to the velocity update at
`0x205950`. With a positive signed step, repeat once for each
millisecond. Subtract each current velocity component from its desired
component in binary32, multiply the difference by binary32 0.05
(`0x3D4CCCCD`), then add it to the current component in
binary32. The new components are stored at record `+0x3C`
and used for the next iteration. The helper performs no velocity
write for a signed step of zero or less. The aim vector, fixed
difference and desired velocity are formed once for the visit,
with no additional draws or reads of the source inside the loop
(`0x220C08–0x220C6A`, `0x205950–0x205A1B`).

### 1.2 Return velocity and catch blend

For ages greater than 800 through 1,100, subtract the
projectile's fixed position from the source's live fixed
position. Convert the signed fixed difference to binary32
and multiply by exactly 1/65,536, then multiply each component
by binary32 ten through `0x0252F0`. Assign that full velocity
to record `+0x3C` before common displacement. This interval
uses neither the aim offset nor the outbound iteration count.
Assignment occurs even for a zero step duration
(`0x220BAC–0x220BF1`, `0x02C4A0–0x02C4FD`,
`0x0252F0–0x025324`).

For ages greater than 1,100 through 1,200, subtract 1,100
as a signed integer, convert to binary32 and divide by
binary32 100. Convert this factor to signed fixed units by
multiplying by binary32 65,536 and truncating toward zero.
The native caller does that same factor conversion twice.
Take the source's live fixed position minus the projectile's
fixed position. Scale each component by the first factor and
then scale the result by the second. Each scale takes the
low 64 bits of the integer product, treats them as signed,
then divides by 65,536 with truncation toward zero. The
first scaled component is therefore an integer before the
second product. Add the final fixed delta to the original
projectile position and store it at record `+0x18`. At age
1,200, both factors are exactly one in fixed units. In the
representable product range this places the projectile at the
source before common displacement. This branch keeps its
existing velocity
(`0x220B0C–0x220BA7`, `0x02C580–0x02C5A9`,
`0x02C6E0–0x02C71C`, `0x02C900–0x02C975`,
`0x28D9E0–0x28DAC1`).

All surviving type-2 branches go directly to common
displacement. Convert the signed step milliseconds to
binary32 and multiply by binary32 0.001 to get seconds.
Multiply each stored velocity component by those seconds,
then add signed step milliseconds to record age. Type 2
bypasses the later common 5,000 ms expiry. A visit beginning
at 1,200 can therefore run displacement and the contact walk
with a new age greater than 1,200. Its next visit first checks
the newly stored age. Contact and sound bucket gates continue
to use the old and new ages described in the held projectile
contract
(`0x20C83D–0x20C86A`, `0x220C9D–0x220CCD`).

## 2. Ordinary impact call for types 0, 1 and 4

### 2.1 Admission and amount

For these types, the direct contact walk can call the ordinary
hit helper for an admitted contact when charge byte `+0x5C`
is zero and variant byte `+0x64` differs from two. Direct
variant 2 has its own output route. The later impact walk
requires creature stop or terrain contact. Type 1 admits that
walk regardless of charge, while types 0 and 4 require nonzero
charge. This second walk uses the ordinary hit helper for
variant 2 as well. It saves the raw charge byte before candidate
iteration
(`0x221971–0x221A1F`, `0x221D03–0x221E28`,
`0x2223B2–0x2223C2`, `0x222457–0x222461`,
`0x2224E2–0x2224EB`).

Each admitted impact candidate starts with a false critical
byte. A resolved source calls `0x0444A0` at `0x2228C7`,
including for a connected nonlocal source. Its draw at
`0x0444E5` and strict probability comparison are the same
as for direct contact.
Read binary32 base amount from record `+0x48` after that
critical result. Multiply by binary32 two only if critical.
The impact amount then reaches the hit helper without a floor
or a distance multiplier. An ordered negative base amount
therefore arrives negative. Direct contact uses the same
base amount and critical factor but floors an ordered negative
result to zero before its ordinary or variant-2 route
(`0x221B50–0x221B8A`, `0x221CF3–0x221D03`,
`0x2228AD–0x22290C`, `0x222A96–0x222AB8`).

With a connected nonlocal or missing source the later
ownership branch skips `0x0441A0` before its draw. For
comparison at the same call boundary, a local source can
reach it, as can an offline nonlocal resolved source with
nonzero controller kind. It receives binary32 0.25.
A true result with saved charge zero, type 1 and variant 1
passes a 30,000 ms type-9 buff to the source's apply helper
and appends a source-addressed buff record. It then appends
sound 47 at the source's live position with pitch and volume
one. This branch leaves the saved amount and raw strength
arguments intact. Its classifier internals are outside this ruling
(`0x22290C–0x222A52`).

### 2.2 Strength, charge and descriptor inputs

Before either candidate walk, the projectile copies its
current velocity for the impulse. When the binary32 squared
length is ordered positive, the impulse copy is normalized.
The helper sums the binary32 component squares, takes a
binary64 square root, rounds to binary32, divides binary32 one
by that length and multiplies the three components by the
quotient. Then multiply the copy by binary32 five times
record strength `+0x54`, and overwrite Z with binary32
three times that same strength. The two walks use this
same prepared impulse. The separate hit-helper strength
argument is the raw binary32 record field. It carries no
factor of three or five
(`0x2214E0–0x221568`, `0x0240F0–0x024169`).

The world is the receiver of `0x196D30`. The impact call at
`0x222AB8` supplies these explicit arguments in native order.
The ordinary direct call at `0x221E8C` has the same argument
order and fields, with its direct amount and a fresh charge
byte from the record.

| Order | Native argument |
| ---: | --- |
| 1–2 | Live target pointer, then resolved live source pointer or null. |
| 3–4 | Prepared binary32 amount, then the critical result byte. |
| 5–6 | Raw charge byte saved from record `+0x5C`, then raw binary32 strength from `+0x54`. |
| 7–8 | Address of the current projectile position at record `+0x18`, then address of the prepared impulse. |
| 9–10 | Current frame output collector, then the shared combat scratch. |
| 11–12 | Type-1 flag, then a 32-bit action argument of zero. The flag is one for type 1 and zero for types 0 and 4. |
| 13–14 | Current resident projectile descriptor, then the enabled flag of one. |

The iterator access `0x02C6C0` supplies argument 13 as the
current resident node address plus eight, the start of the
same 112-byte projectile record. Argument 7 points inside
that record. The original hit helper's decoded body has no
read of either entry argument. Its impulse comes from
argument 8 and its ordinary appended hit position comes from
the live target. There is no descriptor-based rescaling in
this callee at this image boundary
(`0x221E28–0x221E8C`, `0x222A52–0x222AB8`,
`0x196D30–0x1986F9`, `0x197FA8–0x1980BF`).

Within the common helper, target health and the connected
local-target check can return false before shared hit work.
If ordinary defense is reached, the type-1 flag selects
the defense statistic at `0x0467A0`. Flag zero uses
`0x03CFF0`. The defense reduction and its own amount floor
are later than the call boundary. The charge byte is tested
for enhancement work and is carried to ordinary hit byte
`+0x44`. The strength argument contributes to the later
stun calculation and can be replaced there by the live
source-action rules. The final enabled flag admits the
guard check. A true result from the impact call sets the
current substep success indicator. A false result preserves
it, and the candidate walk advances
(`0x196D7F–0x196DAE`, `0x19701B`, `0x19750B–0x19754D`,
`0x197661–0x1976D8`, `0x19804D–0x1980BF`,
`0x1983EC–0x198563`, `0x222ABD–0x222AF7`).

## Evidence limits

The steering ruling starts with the live source aim fields and
covers the ordinary finite components and signed millisecond
inputs. Their aim producers, malformed float conversions and
downstream projectile rendering are outside scope. The impact
ruling pins the ordinary call and its immediate result, including
the input handling in the shared helper. Its complete defensive,
equipment, missing-source and source-action-specific branches,
and the dedicated image, remain open.
