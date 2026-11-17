Status: FINAL / STABLE / HELD

# Alpha ruling: held-spirit light slot factors and product order

Scope: the four equipped-item light blocks of the client frame renderer (routine at RVA
0x0AC260), VA 0x4B0184 ... 0x4B06B3 (RVA 0x0B0184 ... 0x0B06B3), executed once per creature
admitted to the creature light group. Image: client Cube.exe (SHA-256
84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF), 32-bit, image base
0x400000. Addresses below are VAs; RVA = VA - 0x400000. The server image has no counterpart
(rendering only).

## Ruling

1. **Ratio.** rho = (signed 32-bit creature +0x70 converted to binary32) divided by (the
   signed 32-bit result of helper 0x444CB0 converted to binary32): one binary32 division,
   no clamp (0x4B0186 ... 0x4B01A6). rho is computed once and reused by all four blocks.
2. **Exact-zero gate.** rho is compared with binary32 0.0 (literal at 0x745D84, read from
   the image: 0.0) by an unordered compare; all four blocks are skipped (jump to 0x4B06B3)
   only when the compare reports ordered equality (0x4B01AA ... 0x4B01BB). So +0.0 and -0.0
   skip; every other value, negative ones included, proceeds; a NaN rho also proceeds.
3. **Slot order and gates**, each block independent (a failed gate falls through to the
   next block):

   | Order | Slot | Item at creature | Type gate | Alpha gate | Factor (bits) | Factor push site |
   |---|---|---|---|---|---|---|
   | 1 | 7, main hand | +0xAA8 | byte == 3 (0x4B01C9) | A.a > 0 (0x4B01F1 / 0x4B01F8) | **0.3** (0x3E99999A) | 0x4B02C9 ... 0x4B02D8 |
   | 2 | 6, off hand | +0x990 | byte == 3 (0x4B0346) | A.a > 0 (0x4B036E / 0x4B0375) | **0.5** (0x3F000000) | 0x4B0446 ... 0x4B0455 |
   | 3 | 5, shoulders | +0x878 | byte != 0 (0x4B04C3) | A.a > 0 (0x4B04EB / 0x4B04F2) | **0.1** (0x3DCCCCCD) | 0x4B0549 ... 0x4B0558 |
   | 4 | 2, chest | +0x530 | byte != 0 (0x4B05BF) | A.a > 0 (0x4B05E7 / 0x4B05EE) | **0.1** (0x3DCCCCCD) | 0x4B0645 ... 0x4B0654 |

   A is the averaged spirit colour of that slot's item (helper 0x4C6CC0 called with the
   item as receiver); A.a is its fourth component. The alpha test is "branch away if
   A.a <= 0 or unordered", so a NaN A.a skips the block.
4. **Product order.** In every block the light colour is
   C = (A.rgb x rho) x factor, per channel, two binary32 multiplications, rho first:
   * the first product is helper 0x451510 (three binary32 MULSS, operand x rho) at 0x4B0307,
     0x4B0484, 0x4B0587, 0x4B0683;
   * the second product is helper 0x4252F0 (three binary32 MULSS, receiver x scalar,
     callee pops two arguments) at 0x4B0311, 0x4B048E, 0x4B0591, 0x4B068D.
   The slot factor is the scalar of the 0x4252F0 call although it is pushed earlier: the
   push of the factor and of the destination precedes a call to 0x458DE0, which only returns
   its receiver (MOV EAX,ECX; RET, no stack cleanup), so both pushed words stay on the stack
   until 0x4252F0 consumes them. Only the three colour components are multiplied; A.a does
   not enter C.
5. **Radius** 10 (0x41200000) in all four blocks (stores at 0x4B02A4, 0x4B0421, 0x4B0524,
   0x4B0620). Each surviving block appends one candidate (0x4C11F0 at 0x4B0332, 0x4B04AF,
   0x4B05B2, 0x4B06AE) in the order of the table.

Constant bit patterns at the four push sites and both radius stores were read directly from
the image bytes (C7 04 24 9A 99 99 3E; C7 04 24 00 00 00 3F; C7 04 24 CD CC CC 3D twice).

## Which contract wins

* **Right:** `alpha-materials-wildlife-routes.md` section 1.5 "Held-spirit light" and its
  Re-certification (2026-11-16) corrections 1, 2 and 3: main hand 0.3, off hand **0.5**,
  shoulders and chest 0.1; rho first, slot factor second; exact-zero rho test; blocks in
  order 7, 6, 5, 2; hand slots gate on type 3, shoulders/chest on any non-zero type. Its
  worked example 2 (main hand (0.225, 0.1125, 0.0225), off hand (0.375, 0.1875, 0.0375))
  follows from this ruling.
* **Wrong in one cell:** `alpha-terrain-light-source-lifecycle.md`, creature-site table
  row 12 (item at +0x990): "multiplied first by Q, then by 0.3" must read "... then by
  0.5". Its rows 11, 13 and 14 (0.3, 0.1, 0.1), the Q-first order, the exact-zero
  description of Q and the table order agree with this ruling.

Additional precision, not contradicted by either contract: the zero gate lets a NaN ratio
through, while the alpha gate rejects a NaN alpha; negative zero is treated as zero.

## Evidence

Static instruction listing of the client image over 0x4B0157 ... 0x4B06C9, read
instruction by instruction with stack-depth tracking (rho stored at [ESP+0x28] at 0x4B01B1
and re-read at [ESP+0x30] after the two pending pushes at 0x4B02F2 / 0x4B046F / 0x4B0572 /
0x4B066E); bodies of helpers 0x4252F0, 0x451510, 0x458DE0, 0x411E30 (three-word copy),
0x428970 (component address) and 0x4C11F0 (48-byte append) read from the listing; literals
read from the image. Reconstruction sources were not consulted.

## Unresolved

* Static reading only; not verified at runtime.
* Behaviour for a zero denominator from 0x444CB0 (rho infinite or NaN) is described only as
  far as the two gates go; downstream use of a non-finite colour is not covered.
* Light positions (hand attachments +0x1A04 / +0x1B84 with the +0x1D18 / +0x1D20 shifts,
  main position +0x10 for slots 5 and 2) and the view/culling helper calls inside each block
  (0x42C7A0, 0x43B4A0, 0x4120F0) were not re-audited for this ruling; in particular the
  receiver register for the culling helper in blocks 3 and 4 is set by the earlier blocks
  and its value when both hand blocks are skipped was not traced.
* Whether slots 5 and 2 can ever hold spirit cubes in normal play is outside this ruling.
