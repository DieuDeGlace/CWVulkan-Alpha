Status: FINAL / STABLE / HELD

# Alpha ruling: third (extra) corridor line test in the dungeon corridor routine

Scope: the dungeon corridor routine (client RVA 0x1C52A0, server RVA 0x0F9010), the test that
decides whether the third line (coordinate m + 1) is carved, in all four cases: along y with B
lower, along y with B higher or equal, along x with B lower, along x with B higher or equal.
Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, image base 0x400000. RVAs below are image-relative.

Conventions as in alpha-open-points-7.md: the routine receives four boxes in the order PA, SA,
PB, SB (A = the link's first room, B = its second); "B lower" means PB.x < PA.x in the along-x
case and PB.y < PA.y in the along-y case; m is the clamped corridor coordinate (an x
coordinate in the along-y case, a y coordinate in the along-x case). All arithmetic is 32-bit
signed; every comparison below is strict and signed.

## Ruling

The third line is carved only if the second draw of the link is odd **and** both bounds below
hold. If the draw is even or either bound fails, nothing more is carved for that link; the
draw has already been made in every case.

| Case | First bound | Second bound |
|---|---|---|
| Along y, B lower | m < PA.x + SA.x - 1 | m < PB.x + SB.x - 1 |
| Along y, B higher or equal | m < PA.x + SA.x - 1 | m < PB.x + SB.x - 1 |
| Along x, B lower | m < PA.y + SA.y - 1 | **m < PB.x + SB.x - 1** |
| Along x, B higher or equal | **m < PA.x + SA.x - 1** | **m < PB.x + SB.x - 1** |

So in the along-x case, although m is a y coordinate, the third-line test uses B's **x**
extent in both branches, and A's **x** extent when B is higher or equal; only the first bound
of the B-lower branch uses a y extent (A's). The first bound is checked before the second;
both are pure reads with no draws, so the order has no observable effect.

The second-line test (coordinate m - 1) is not affected: it is m > PA and m > PB on m's own
axis (x for along y, y for along x) in all four cases.

## Which contract statement is right

* **Right:** alpha-open-points-7.md §2.3 (table and closing paragraph) and its summary item 3:
  the along-x third test compares m with x extents, one bound when B is lower, both when B is
  higher or equal. Its along-y rows and every second-line row are also right.
* **Wrong as a statement of native behaviour:** alpha-worldgen-structures.md §1.6, "the same
  with the roles of x and y exchanged", insofar as it implies the along-x third-line test is
  m < PA.y + SA.y - 1 and m < PB.y + SB.y - 1. That exchange holds for the overlap test, m,
  the clamp, the end points, the second-line test and the first bound of the B-lower third
  test, but not for the other three third-test bounds of the along-x case, which use x
  extents as tabled above. Its along-y description is right.

## Evidence

Client routine 0x1C52A0 .. 0x1C56DA (returns popping 16 bytes of arguments: four box
pointers). Argument slots: first = PA, second = SA, third = PB, fourth = SB (overlap test at
0x1C52BE .. 0x1C52D9 forms PA.x + SA.x and PB.x + SB.x from them).

| Case | Odd-draw gate | First bound read | Second bound read | Server RVA of same test |
|---|---|---|---|---|
| Along y, B lower | 0x1C538A .. 0x1C539C | 0x1C53A2 .. 0x1C53AC: SA field 0 minus 1 plus PA field 0 | 0x1C53B2 .. 0x1C53BC: SB field 0 minus 1 plus PB field 0 | 0x0F9112 .. 0x0F912C |
| Along y, B higher or equal | 0x1C546A .. 0x1C547C | 0x1C5482 .. 0x1C548C: SA field 0 minus 1 plus PA field 0 | 0x1C5492 .. 0x1C549C: SB field 0 minus 1 plus PB field 0 | 0x0F91F2 .. 0x0F920C |
| Along x, B lower | 0x1C5593 .. 0x1C55A5 | 0x1C55AB .. 0x1C55B9: SA field +4 plus PA field +4, minus 1 | 0x1C55BF .. 0x1C55CD: PB field 0 plus SB field 0, minus 1 | 0x0F931B .. 0x0F933D |
| Along x, B higher or equal | 0x1C566E .. 0x1C5680 | 0x1C5682 .. 0x1C568C: SA field 0 minus 1 plus PA field 0 | 0x1C568E .. 0x1C569A: SB field 0 minus 1 plus PB field 0 | 0x0F93F2 .. 0x0F940A |

Field 0 is x and field +4 is y of a three-integer box vector (the overlap tests at 0x1C52C9 and
0x1C54C9 read field 0 and field +4 respectively). Each bound is followed by a
branch-if-greater-or-equal on m to the routine's exit (0x1C56CA), so each test needs m strictly
less than the bound. The gate is the signed remainder of the import `rand` (client IAT slot RVA
0x2FC468, server 0x1582F4) by 2; zero skips to the exit.

Supporting reads, consistent with the ruling:

* In the along-x B-lower branch the sum PB.x + SB.x formed for the second bound (0x1C55C6) is
  reused unchanged as the third line's end x at B (0x1C55D6), so the x read is not a
  mis-decoded y.
* In the along-x B-higher branch the value PA.x + SA.x - 1 formed for the first bound
  (0x1C5682 .. 0x1C5688) is reused as the third line's start x at A (0x1C56B1), and PB.x - 1
  is formed from the PB.x read of the second bound (0x1C5691, 0x1C569C) as the end x.
* The along-x m is formed from y fields (0x1C54EB .. 0x1C5503: SB field +4 halved, plus PB
  field +4, clamped to PA field +4 .. PA.y + SA.y - 1), and the second-line test of the along-x
  case reads field +4 of PA and PB (0x1C5555 .. 0x1C555D, 0x1C562C .. 0x1C5634). Only the third
  test departs from the y fields.

Server: routine 0x0F9010 has the same 395 instructions as the client routine, identical byte
for byte except the security-cookie address, the `rand` slot, the line-routine call targets
(server line routine RVA 0x1234B0) and the cookie-check call. Its only caller is the server
dungeon builder (server RVA 0x100300, call site 0x1021CE); the client routine's only caller is
the client builder (client RVA 0x1CC510, call site 0x1CE3DE). The server zone generator
therefore makes exactly the same comparisons.

Method: static reading of the per-instruction listings of both original images; callers and
the `rand` slots taken from the listings' call and symbol tables. Reconstruction leads
(Infinite-Cubes/alpha) were not consulted for this ruling.

## Unresolved

* Not verified at runtime; this is a static reading of both images.
* Whether the x extents in the along-x third test were intended by the original authors is
  not knowable from the images; the ruling records what the code does.
* No seed was examined to show how often the departure from the exchanged-axis reading
  changes carved cells; that depends on room boxes and is not established here.

## Re-certification (2026-11-16)

Re-checked against the `Cube.exe` and `Server.exe` instruction listings only, after writing,
by full readback of this file.

**Confirmed:** routine bounds, argument order PA, SA, PB, SB; the x-first overlap order; the
odd-draw gate before each third test and the draw being made regardless of the bounds; all
eight third-test bounds and their addresses in both images; along-y bounds on x in both
branches; along-x bounds as A's y and B's x when B is lower, and A's x and B's x when B is
higher or equal; reuse of the bound sums as the third line's end points; the second-line
tests on m's own axis; client and server routines identical apart from absolute addresses;
single caller in each image.

**Corrected:** none (new file).

**Removed:** none.

**Unresolved:** as listed above.
