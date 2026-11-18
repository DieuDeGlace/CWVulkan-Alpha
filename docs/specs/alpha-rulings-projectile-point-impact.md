# Alpha projectile point-cell impact and defensive hit output

Status: FINAL / STABLE / HELD

## Scope and evidence

This ruling supplements `alpha-connected-projectile-receipt-step.md`,
`alpha-rulings-projectile-ray-and-variant.md` and
`alpha-dedicated-projectile-step.md`. Its addresses are RVAs in
the following original 32-bit images.

| Image | Size in bytes | SHA-256 |
| --- | ---: | --- |
| Cube.exe | 3,885,568 | 84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf |
| Server.exe | 1,718,784 | 1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1 |

## 1. Point lookup before the backward ray

The terrain stage first converts the prepared substep displacement
to signed fixed units and adds it to the resident position at
projectile `+0x18`. The first point lookup uses that advanced
position. Each position component is signed 64-bit, at 65,536
units per block. Both projectile call sites supply a null chunk
hint. The result's byte `+0x03` is masked with `0x1F`. Types 0
and 2 pass to the backward ray, and every other value takes
the solid-point path. The ray receives the advanced position
only on the passable result.

| Native boundary | Client RVA | Dedicated RVA |
| --- | --- | --- |
| Position increment | `0x221FD7–0x221FE8` | `0x147D97–0x147DA8` |
| Advanced-position point call | `0x2220D1` | `0x147E91` |
| Fixed-position point lookup | `0x02F860–0x02F90C` | `0x006050–0x0060FC` |
| Integer-cell lookup | `0x02F7E0–0x02F854` | `0x005FD0–0x006044` |
| Column lookup with optional chunk hint | `0x0347A0–0x03486D` | `0x006100–0x0061CD` |
| Cell-type classification | `0x03B480–0x03B495` | `0x0061F0–0x006205` |
| Backward ray call | `0x222293` | `0x148053` |

### 1.1 Signed cell coordinates

In each image the point wrapper subtracts 65,536 from each
negative original fixed component. It then performs signed
64-bit division by 65,536, truncates toward zero, and passes
the low 32 bits of each quotient as the signed cell coordinate.
Nonnegative components take the same division without the
subtraction. In the ordinary coordinate range an exact
negative plane therefore resolves to the cell below the plane.

| Fixed Z | Z in blocks | Integer cell Z |
| ---: | --- | ---: |
| -65,537 | One fixed unit below -1 | -2 |
| -65,536 | Exactly -1 | -2 |
| -65,535 | One fixed unit above -1 | -1 |
| 0 | Exactly 0 | 0 |

The positive exact-plane case resolves to the cell starting
at that coordinate. The integer column helper bounds X and Y
to nonnegative values strictly below 16,777,216 before the
chunk lookup. The null chunk hint selects lookup by X and Y
divided by 256. The resulting chunk's `+0xA8` array uses a
32-byte column at index X remainder plus 256 times Y remainder.

### 1.2 Cell selection and fixed fallback records

First, a missing column or a signed Z below column base
`+0x10` selects the same solid fallback. With a column present,
compare signed Z against base plus the cell count at column
`+0x1C`. At or above this upper end, select the upper fallback
by whether signed Z is strictly positive. For an in-range Z,
read the four-byte cell at column `+0x18` indexed by Z minus
base. If its low five type bits are zero, Z is at most zero,
and bit `0x40` is clear in its type byte, substitute the
at-or-below-zero fallback. Otherwise retain the stored cell.

| Selection | Bytes at cell offsets `+0x00–0x03` | Client record RVA | Dedicated record RVA |
| --- | --- | --- | --- |
| Missing column or below base | `C8 C8 C8 01` | `0x36AFC4` | `0x183D14` |
| At or above upper end with positive Z | `FF FF FF 00` | `0x36AFC0` | `0x183D10` |
| At or above upper end with nonpositive Z, or in-range replacement | `FF FF FF 82` | `0x36AFBC` | `0x183D0C` |

A missing chunk or an out-of-range horizontal coordinate
returns no column. Its fallback has low-five-bit type one,
so the advanced-position test selects a solid impact.
The upper fallback types reduce to zero and two and admit
the ray. The type bytes and three color bytes above are
set by the original initializers at client
`0x2F9720–0x2F97A7` and dedicated `0x1569B0–0x156A37`.

## 2. Deterministic solid-point effect

A solid advanced sample rolls back the just-added fixed
substep displacement and sets the terrain-contact flag.
The rollback uses the fixed conversion of the same prepared
substep vector. A type-0 projectile then performs a second
point lookup. The coordinate accessors returned addresses of
the live position components earlier. They still point into
the resident projectile after rollback. The second lookup
therefore reads the restored position. Its cell supplies the
color, and that same restored fixed position is copied into
the effect. The advanced cell determines whether this branch
runs. The two cell lookups can return different cells.

Read unsigned bytes `+0x00`, `+0x01` and `+0x02` from the
second cell in that order, convert each to binary32, and divide
each separately by binary32 255. The fourth color component
is binary32 one. There is no second solidity gate for the
color sample. A missing or below-base result on the second
lookup supplies the three color inputs 200. It is the outcome
of the second lookup that determines these inputs.

| Effect offset | Width | Stored value |
| --- | ---: | --- |
| `+0x00` | 24 | Restored projectile position, three signed 64-bit fixed components. |
| `+0x18` | 12 | Binary32 velocity (0, 0, 10). |
| `+0x24` | 16 | Binary32 color from the second sample, with fourth component 1. |
| `+0x34` | 4 | Binary32 0.1, bits `0x3DCCCCCD`. |
| `+0x38` | 4 | Effect type 4. |
| `+0x3C` | 4 | Zero from the record initializer. |
| `+0x40` | 4 | Binary32 3.0 from the record initializer. |

Append one record to collector `+0x10`. The append copies
the fields through `+0x40`. This construction spends no
random draw. Its type-0 check follows the point rollback
and precedes other source-retime and impact work. The
separate ray-return rollback only changes position and sets
terrain contact.

| Effect evidence | Client RVA | Dedicated RVA |
| --- | --- | --- |
| Live coordinate accessor | `0x0120B0–0x0120BA` | `0x0E2320–0x0E232A` |
| Solid-point rollback | `0x2220E5–0x22210E` | `0x147EA5–0x147ECE` |
| Second point lookup | `0x222157` | `0x147F17` |
| Unsigned color and component division | `0x050F00`, `0x069020` | `0x0C6470`, `0x0F7A70` |
| Effect initializer | `0x065ED0–0x065EE0` | `0x0C8510–0x0C8520` |
| Field writes and append | `0x22216F–0x222234` | `0x147F2F–0x147FF4` |
| Effect field copy | `0x065E60–0x065ECD` | `0x022E10–0x022E7D` |

## 3. Dedicated defensive hit records and application

The dedicated ordinary hit helper at `0x0CFD50` has four
traced calls to the normal record applier `0x0CEA80`.
Each follows an initial append to the collector's hit list.
The mode byte at world `+0xB4` must be zero for the apply
call. Each call passes the same record and collector that
received the first append. The dedicated world starts with
that mode and a null local pointer at `+0xB8`.

| Shared helper branch | Hit type | First append | Apply call |
| --- | ---: | --- | --- |
| Kind-1 target protection | 3 | `0x0D000E` | `0x0D002F` |
| Active guard | 1 | `0x0D01C0` | `0x0D0249` |
| Positive effective ward | 5 | `0x0D041A` | `0x0D04FD` |
| Ordinary result | 0 | `0x0D1697–0x0D16D2` | `0x0D16F0` |

The applier first resolves the full target key from hit
`+0x08` in the live world map. It exits if the lookup has no
actor or binary32 health is ordered at most zero. Then, if
world `+0xB8` is null and target controller byte `+0x60`
equals zero, it appends a second copy of the supplied record
to the hit list and returns before the remaining application.
The hit-type byte has no gate before this second append.
Duplicate entries retain their order through the collector
range copy to the peer hit queue
(`0x0CEACB–0x0CEB3D`, `0x02736A–0x027392`,
`0x020EF0–0x020F77`).

- Kind-1 target protection requires controller kind 1 and
  either mask `0x80` in the low byte at target `+0x124` or
  mask `0x20` in target byte `+0x5C`. It draws and appends
  sound 24, appends the type-3 hit, calls the applier and
  returns false. That live kind-one gate excludes a kind-zero
  duplicate from this branch. The earlier source-action 74/91
  contact rejection is a separate exit. It rejects when target
  `+0x5C` low bit is clear and reaches no hit append or apply
  call (`0x0CFDD4–0x0CFDF1`, `0x0CFEE1–0x0D0036`).
- Active guard requires the helper's enabled flag, signed
  target `+0x130` at most zero and an admitted target action.
  The guard predicate admits 8, 71, 72 and 98, plus 59 and 63
  when target class and specialization are both one. It also
  requires ordered positive guard value at `+0x174`. This
  route mutates guard, mana and pending resource, appends
  the type-1 record, draws and appends sound 24, then calls
  the applier and returns false. It has no kind-one requirement,
  so a qualifying living kind-zero target gets the second type-1
  record after the guard work
  (`0x00F610–0x00F64B`, `0x0D003B–0x0D0250`).
- A positive effective ward appends the type-5 record, draws
  and appends sound 91, then calls the applier. A qualifying
  living kind-zero target gets the second type-5 record. The
  caller subsequently subtracts the effective ward from the
  incoming amount. For ordinary finite values, a nonpositive
  remainder returns true, and a positive remainder continues
  through the remaining hit work. A later ordinary record
  can therefore also acquire its own second copy
  (`0x0D0255–0x0D052B`).

The direct projectile variant-2 beneficial branch is a
separate caller at `0x147BD4`. Its apply path remains as
described in the dedicated projectile contract.

## Evidence limits

The point and effect rulings start from a resident projectile
and its prepared substep vector. Launch producers and particle
presentation are outside scope. The defensive ruling covers
the first append, the normal apply boundary and the role-zero
second copy. It leaves other branches of record application
and any upstream network action admission open.
