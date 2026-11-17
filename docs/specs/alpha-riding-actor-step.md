# Alpha riding actor step

Status **FINAL / STABLE / HELD**

Evidence is from the original x86 images. Routine addresses are RVAs.
Both images have base 0x400000. Creature offsets start at the whole
creature object. Positions use 65,536 fixed units per block.

| Image | SHA-256 | World step |
|---|---|---:|
| C (Cube.exe) | 84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf | 0x20c510 |
| S (Server.exe) | 1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1 | 0x1322d0 |

## 1. Action entry and step time

The client Ride prompt arm requires the current interaction target's
controller-kind byte +0x60 to equal 5, its type at +0x64 to be rideable,
and its reverse owner ID at +0x198 to match the local creature's ID
at +0x08, comparing both words. Local signed riding level +0x113c must
be nonzero and the local action byte +0x68 must differ from 106.
With that arm reached, the input requires game byte +0x0f nonzero
and byte at C 0x36b138 zero. The frame writes its current +0x0f sample
to 0x36b138 later in the same update. The latter test is the previous
R sample, giving a rising edge. Admission writes action 106 and zeros
the signed 32-bit action clock at +0x6c. It occurs after the client
frame's world pieces.
(C 0x096a59–0x096b19, 0x09b593–0x09b59b, 0x08cef4–0x08cf49.)

Both images obtain the same binary32 seconds value for the world piece.
Convert the signed 32-bit millisecond argument to binary32, then multiply
by binary32 0.001 and round to binary32. The speed-stage drain and the
terrain refill each use this seconds value. The client divides its
world time into pieces of at most 20 ms.
(C 0x20c549–0x20c54f, 0x20c83d–0x20c862, 0x2fd870.
S 0x1325fd–0x132622, 0x158858.)

## 2. Speed, drain and dismount

At the speed-limit stage the riding branch requires the current actor
to equal the local creature pointer at world +0xb8 and action +0x68
to equal 106. This stage reads the forward pet ID at creature +0x11c8.
It resolves the ID, checks that the resolved pet's binary32 health
+0x16c is ordered and at least zero, and checks the pet's type +0x64
against the rideable table. The native implementation resolves the ID
again for each of the health and type checks it reaches. The gate uses
the type and health of the resolved actor. It does not inspect that
actor's role or reverse owner field.
(C 0x2192d9–0x219359, 0x02f000. S 0x13f099–0x13f119, 0x005420.)

The rideable IDs in both images are 19–20, 22–23, 25–40, 63–67, 74–75,
98–100, 102–105 and 151. For a valid pet, the signed riding level at
rider +0x113c supplies a speed factor. A level below 1 yields zero.
Otherwise convert the level to binary32, multiply by binary32 0.1
and add 1. Take its reciprocal, subtract that from 1, then add 1.
Use binary32 operations in that order. The nonzero factor multiplies
the current temporary speed scalar in binary32. The factor branches
each resolve the pet once more before their common exit.
(C 0x044760–0x044785 and table at 0x044790, 0x046aa0–0x046ae5,
0x21935f–0x2193da. S 0x00f7f0–0x00f815 and table at 0x00f820,
0x0116f0–0x011735, 0x13f11f–0x13f19a.)

A missing pet, rejected health or type, zero factor, or failed final
lookup clears the rider's action byte to zero. Once the riding branch
has been entered, those routes still reach the following tail in the
same visit.

1. From local creature +0x40 calculate the binary32 squared intent
   magnitude. Square X, Y and Z separately in binary32 and add X and Y
   first. If the magnitude is strictly positive, subtract binary32
   0.002 times the piece's binary32 seconds from energy +0x1198,
   preserving the binary32 multiplication and subtraction.
2. Test the resulting or current energy even when the subtraction was
   skipped. Energy at most zero writes positive zero to +0x1198 and
   clears action +0x68.
3. Test bit 0 of the rider's byte at +0x124. A set bit clears action.

The tail does not check action 106 again. The speed-stage cancellations
write the action byte without writing +0x6c at these sites. The standard
dedicated world starts with a null local pointer, so its per-actor
processing skips this local speed and drain arm.
(C 0x2193b0–0x21944f, 0x024860–0x024891, 0x2fcd98.
S 0x13f170–0x13f20f, 0x0021b0, 0x1586a0, 0x149cb7–0x149cbd.)

## 3. Terrain refill

The collision and terrain portion of a creature visit clears a local
mark before the liquid and ground probes. The probes share that mark.
The block-kind helper extracts the low five bits of the block's byte
at offset +3.

- The earlier box liquid scan latches the mark if a visited block has
  kind 2 and sets bit 0x02 at creature +0x5c. It can see several such
  cells. Controller kind 6 skips this scan.
- When bit 0x02 is clear, the ground probe checks the inclusive X and Y
  box spans at a common ground Z. The probe sets ground bit 0x01 for a
  cell whose kind is neither 0 nor 2, and latches the refill mark for
  kind 3. It completes each visited X column's Y loop, then stops
  advancing X once the ground bit is set. The latch for kind 3 covers
  the cells actually visited.
- A set water bit skips the ground probe and leaves the earlier
  kind-2 mark available to the same refill tail.

The ground probe forms its fixed Z by subtracting the separately
converted binary32 half-height and binary32 0.2 from the current
fixed position Z. Each subtraction converts its distance by a
binary32 multiplication by 65,536, followed by truncation to signed
64 bits. The ground cell conversion divides the fixed result by
65,536 toward zero and subtracts one for a negative input. This also
subtracts one for a negative exact multiple.
(C 0x21cf80–0x21d0cc, 0x21d1df–0x21d322, 0x012220–0x012255,
0x02f100–0x02f135, 0x034c10, 0x03b480.
S 0x142d40–0x142e8c, 0x142f9f–0x1430e2.)

After the scans converge, one refill attempt is possible for the
current creature visit. A set mark and current controller-kind byte
5 cause a world lookup using the two-word reverse owner ID at +0x198.
A non-null owner with ordered binary32 energy +0x1198 below 1 receives
one increment. Multiply binary32 0.05 by the piece's binary32 seconds,
then add to the owner's energy in binary32 and store. If the resulting
value exceeds 1, replace it with binary32 1. The attempt occurs once
after the scans, independent of the number of matching cells. Its
owner test does not require action 106, a matching forward ID,
a particular owner role or a nonzero numeric owner ID.
(C 0x21d322–0x21d385, 0x02f000, 0x2fcdb4.
S 0x1430e2–0x143145, 0x005420, 0x1586b8.)

The refill is in the scanned actor's physics portion, before the
step-down and subsequent wall probes. The riding drain is earlier
in the main body of a rider's own visit. The world advances the main
creature iterator between visits. The relative ordering of one
creature's refill and another creature's drain follows that iteration,
with each using the energy present when it reaches its store.
(C 0x20c555–0x20c558, 0x2193de–0x21944f, 0x21d38f–0x21d7ac,
0x21fe21–0x21fe5b. S 0x14314f–0x14356c, 0x145be1–0x145c1b.)

## 4. Two carried-pet copies

Both copy sites read the current creature role byte at +0x60 and require
5. They resolve the creature named by +0x198 in the world creature map
and require that owner's current action byte to equal 106. Each site
first checks for a resolved owner, resolves again for the action test,
then resolves again for the copy. A numerical zero reverse ID can be
looked up as a map key. The copy gates add no health, owner-role,
forward-ID, or separate fighter-field test. They write neither role
nor action. The late pass re-evaluates the live fields independently.

For each copied position, take half the pet box Z at +0x88 and half the
owner box Z at +0x88 in binary32. Subtract the owner half from the pet
half, add binary32 0.01, then multiply by binary32 65,536 and truncate
to signed 64 bits. Use this as the offset on Z, with zero X and Y.
The copy computes the offset separately for the two position stores.

| Pet fields | Source or assigned value |
|---|---|
| +0x1350, three signed 64-bit components | Owner's displayed position plus offset |
| +0x1374, three binary32 components | Owner's displayed body rotation |
| +0x10, three signed 64-bit components | Owner's live record position plus offset |
| +0x28, three binary32 components | Owner's record orientation |
| +0x1180, +0x1188, +0x118c | Owner's corresponding four-byte values |
| +0x34 and +0x40, three binary32 components each | Positive zero velocity and movement intent |

The first copy is the attach arm of the per-actor displayed-pose stage.
It is reached after that visit's movement, terrain probes and motion
handling, then replaces ordinary displayed-position and body-rotation
synchronization, including its smoothing alternative. The main actor
pass can skip the body at its earlier nonpositive-health check. A
successful copy rejoins at the subsequent +0x138c vector update before
the main iterator advances. This permits the first copy to read an
owner that has yet to take its own turn in the current piece.
(C 0x211adb, 0x21912d–0x219137, 0x21b43a–0x21b44b,
0x21f968–0x21fb75, 0x21fb80–0x21fd54, 0x21fda6–0x21fe5b,
0x02c460, 0x2fcda0. S 0x13789b, 0x145728–0x145935,
0x145940–0x145b14.)

The second copy is in a fresh walk over the world creature map after
the main per-actor loop has finished, before world result packing.
It skips null creature pointers. Each visited actor first has a separate
+0x11d0 link check that can clear that link when its target's health is
nonpositive. The role-5 and owner-action tests follow this check.
The second copy reaches the same pose and motion fields without the
main loop's early health check on the currently visited pet. It does
not run its earlier physics or smoothing again. A later owner action
change can therefore alter whether the second copy runs.
(C 0x220e51–0x221136. S 0x146c11–0x146ef6.)

## 5. Separate static-object link

Before the first pose copy, the world tail has a different transform
arm. It requires a clear world byte at +0xb4, a nonzero two-word
identifier at creature +0x1d30 and +0x1d34, and an entry in the world
object map at +0x0c under that key. This arm can adjust the creature's
live position before reaching the active role-5 checks. The earlier
population site copies these words from +0x10e0 and +0x10e4 of its
source record. The traced R admission and both active role-5 copies
use their own fields and do not establish a nonzero value for this
static-object link.
(C 0x210161–0x210173, 0x21f6da–0x21f956. S 0x14549a–0x145716.)

The full upstream provenance of the optional static-object link,
the exact interpretation of auxiliary fighter state outside the
traced tests, and behavior with concurrent map or actor replacement
remain open.
