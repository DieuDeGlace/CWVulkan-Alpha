Status: FINAL / STABLE / HELD

# Alpha ruling: beam area flag for action 95, and strike-origin height for actions 69 and 75

Scope: two disagreements between held contracts about the world update's calls into the
attack (hit) routine.

1. The area flag passed by the beam call site (actions 28, 94, 95 = 0x1C, 0x5E, 0x5F).
2. The vertical adjustment of the melee strike origin for actions 69 and 75 (0x45, 0x4B).

Images: client Cube.exe (SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF)
and Server.exe (SHA-256 1FFBCBCE251E1506A63C9CE18AB0009D5553FCBB9E6202BE2BC3A7D870D34FB1),
both 32-bit, preferred base 0x400000. Both hashes were recomputed before use. All addresses
below are image-relative RVAs, written client / server. Inside the world update routine the
server RVA is the client RVA minus 0xDA240.

## 1. Beam call: area flag for action 95 (0x5F)

### Ruling

For action 95 the beam call passes the area flag **set (1)**. For actions 28 and 94 it passes
it clear (0). The area value passed beside it is 0.1 for action 95 and 0 for 28 and 94.

* **Right:** alpha-strike-call-sites.md §8 item B5 ("flag B = 1 for action 0x5F, else 0") and
  alpha-attack-routine.md §6 table row 4 ("action is 0x5F or 0x68").
* **Wrong:** alpha-open-points-6.md §1.7 ("area flag clear") and §4.1 ("The beam passes area
  flag clear"). Both must read: area flag set for 95, clear for 28 and 94. Their area value
  statement (0.1 for 95, 0 otherwise) is right.

The flag rule also yields 1 for action 104 (0x68); that branch is not reached from the action
dispatch (alpha-strike-call-sites.md §8 B8), so for the three beam actions the flag is exactly
"action is 95".

### Evidence

* Beam call: client 0x2166C8 / server 0x13C488, both calling the attack routine (client
  0x196D30 / server 0x0CFD50). Immediately before the call (client 0x216666 … 0x216672 /
  server 0x13C426 … 0x13C432) a byte is formed as 1 when the striker's action byte (+0x68) is
  0x5F or 0x68, else 0, and is pushed as the fifth stack argument (client 0x2166AC / server
  0x13C46C). The sixth argument is the area value local, which is set to the constant at client
  RVA 0x2FCB7C / server RVA 0x1586BC (both bytes 0x3DCCCCCD, binary32 0.1) only when the action
  is 0x5F (client 0x2165D4 … 0x2165E0 / server 0x13C394 … 0x13C3A0), and is 0 otherwise.
* That fifth position is the area flag. At the melee call (client 0x21476B) the fifth argument
  is the melee flag stored at client 0x2137F1, which is 1 when the striker's +0x13B4 is greater
  than 0 or the action is 0x0B (client 0x213625 … 0x2137EB): the flag that
  alpha-open-points-6.md §4.1 itself calls the area flag. Inside the attack routine the fifth
  argument is copied into byte 0x44 of the 72-byte hit record being built (client 0x1980A1 …
  0x1980B8 / server 0x0D10C1 … 0x0D10D8; the same record's offsets 0x18 and 0x38 … 0x40 are
  written alongside, fixing the record base), which is the hit-record field
  alpha-open-points-6.md §3.3 names the area flag. The same argument is also forwarded to a
  callee at client 0x197661 / server 0x0D0681 and tested at client 0x1976D8 / server
  0x0D06F8; those uses are as described in the held contracts and were not re-derived here.
* Client and server bodies are instruction-for-instruction parallel over the compared range.

## 2. Strike origin height for actions 69 and 75 (0x45, 0x4B)

### Ruling

The quantity is **subtracted**. For actions 69 and 75, after the forward offset has been
added to the strike origin, the origin's z is lowered by 0.5 × (bz − bx) of the striker,
where bx and bz are the striker's box x (+0x80) and box z (+0x88).

Exact order: (bz − bx) in binary32; times 0.5 in binary32 (constant client RVA 0x345DA0 /
server RVA 0x1586D0, bytes 0x3F000000); converted to the fixed-point position unit (times
65536 in binary32, constant client RVA 0x2FCE7C / server RVA 0x15878C, bytes 0x47800000, then
the runtime float-to-64-bit conversion); then that signed 64-bit value is subtracted from the
origin's 64-bit z with borrow. No draw is made. This step follows the forward offset and the
aim-vector replacement for 30–33 (which does not apply to 69 or 75), and precedes the
replacement for 11, 54 and 61.

So for a striker taller than it is wide the strike origin sits lower than the striker's
position plus the forward offset; for bz < bx it sits higher.

* **Right:** alpha-strike-call-sites.md §4 K2 step 6 ("subtracted from the centre's z").
* **Wrong:** alpha-open-points-6.md §1.4, bullet "69, 75: O.z += 0.5·(bz − bx)". It must read
  O.z −= 0.5·(bz − bx).

### Evidence

* Client 0x213A7E … 0x213ACA / server 0x13983E … 0x13988A. The action byte is compared with
  0x4B and 0x45; on a match the striker's box z is fetched (index 2 of +0x80), box x is
  subtracted from it (the box-x pointer was taken at client 0x213850 / server 0x139610 with
  index 0), the result is multiplied by 0.5 and converted to the fixed unit (client 0x02C580 /
  server 0x002A10), and the routine at client 0x02CA00 / server 0x002EB0 is applied to the z
  component (index 2) of the origin. That routine performs a 64-bit subtraction (low word
  subtract, high word subtract with borrow) of its argument from its target and returns the
  target. By contrast the origin's forward offset and the 30–33 aim offset are applied by the
  64-bit addition routine at client 0x02C800.
* Client and server bodies are parallel (same displacements and stack slots).

## Evidence method

Static disassembly of both original images, read instruction by instruction over the cited
ranges and the helper routines named; constants read from the image bytes through the
section table. Reconstruction sources (Infinite-Cubes/alpha) were not consulted for this
ruling.

## Unresolved

* Not verified at runtime; both rulings are static readings of the two images.
* The downstream effects of the area flag inside the attack routine (the forwarded callee and
  the test at client 0x1976D8 / server 0x0D06F8, and any reader of hit byte 0x44) were not
  re-derived; consult the held contracts for them. With the corrected flag, those effects apply
  to action 95 beam hits.
* The rounding behaviour of the runtime float-to-64-bit conversion used by the fixed-unit
  conversion was not re-derived here; it is taken as the truncating conversion described in the
  held contracts.

## Re-certification (2026-11-15)

Authored and read back in full on 2026-11-15 from the two original images identified above
(hashes recomputed that day). Both conflicts settled: conflict 1 in favour of
alpha-strike-call-sites.md B5 and alpha-attack-routine.md row 4; conflict 2 in favour of
alpha-strike-call-sites.md K2 step 6. alpha-open-points-6.md §1.4, §1.7 and §4.1 are wrong on
the points stated and were not edited. No reconstruction leads, implementation material,
tests, captures or logs were used.
