# Alpha packet 6 bomb defaults ruling

**FINAL / STABLE / HELD**

The original dedicated server and offline client give a newly placed bomb
**500.0 live health**. This ruling supersedes the placement health of 1
in the packet 6 sender-turn contract. It governs the first fuse clamp
described in §3.1 of the bombs and pets contract.
[Packet 6 Kind 1](alpha-packet6-sender-turn.md#kind-1--use-item) ·
[Bombs and pets](alpha-pets-bombs.md)

## Images and scope

- **S** is `Server.exe`, SHA-256
  `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.
- **C** is `Cube.exe`, SHA-256
  `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.

Addresses are RVAs from image base 0x400000. Object offsets start at the live
creature object. Record offsets start 0x10 bytes into that object. This
ruling uses static evidence from the two images for successful type 1,
subtype 7 placement and the first authority fuse turn of the surviving bomb.

## Placement

1. The authority branch chooses a signed creature ID one below the least
   signed key holding a live creature, with zero included as a baseline.
   It constructs the creature for that ID and installs it in the live
   table. The position copy then transfers the sender's three fixed-point
   components. Evidence at
   S 0x136e64–0x136f9c, 0x2a40 and C 0x2110a4–0x2111dc, 0x2c5b0.
2. The branch reads a **signed 16-bit** level from item +0x10, sign-extends
   it and writes all 32 bits of the level at record +0x180 (object +0x190).
   The branch replaces the constructor's level 1 with the item's exact
   value throughout the signed range, including zero and negatives. For
   example, item values 0x0000, 0xffff and 0x8000 produce 0, −1 and
   −32,768. The branch sets creature type 144 at record +0x54 and controller
   6 at record +0x50. Evidence at S 0x71c1, 0x136fa9–0x136fc5 and
   C 0x3c2a1, 0x2111e9–0x211205.
3. The **character class remains 0 (no class)** at record +0x130
   (object +0x140). The constructor zeroes that byte and the adjacent
   specialization byte together. The only field copied from the sender
   is position. Class 0 takes the ordinary unmodified class path in the
   maximum-health routine. Evidence at S 0x7101, 0xfe6f–0xfeca and
   C 0x3c1e1, 0x44e7f–0x44eda. The client also reads object +0x140 as
   the class selector for specialization labels at C 0xd535a–0xd5362 and
   0xa63cd–0xa6433.

The object constructors at S 0x6400 and C 0x3b690 call their record
initializers at S 0x7020 and C 0x3c100 with the base advanced by 0x10
(calls at S 0x6441 and C 0x3b6d1). The stores at S 0x715a and C 0x3c23a
put binary32 500.0 (0x43fa0000) at **record +0x15c, object +0x16c**.
The adjacent 1.0 store at S 0x716e and C 0x3c24e uses **record +0x16c,
object +0x17c**, a stat multiplier. Health is 500.0 when the bomb enters
the table and when the placement handler finishes. The maximum-health
clamp and fuse both access that same object +0x16c.

Placement consumes **five** MSVCR110 `rand` draws. The appearance
initializer receives no seed record. The first three draws supply the
temporary third, second and first seed bytes, respectively, modulo 256.
The fourth and fifth draws supply the two values modulo 100. The fixed
type-144 appearance uses model 0x9f2 and consumes zero further draws.
Evidence at S 0x136fc5, 0xa840–0xa8f7, 0xef60, 0xe758–0xe7ba and
C 0x211205, 0x3f7c0–0x3f877, 0x43ee0, 0x436d8–0x4373a.

## First fuse turn

The bomb's signed ID places it behind the current cursor in the ascending
creature sweep. Its first scheduled actor turn is in the next authority
step. The current iteration continues with the sender. Evidence at
S 0x136290, 0x136f8d, 0x137856, 0x145be1–0x145c1b, 0x130600 and
C 0x2104d0, 0x2111cd, 0x211a96, 0x21fe21–0x21fe5b.

At the start of the bomb's health processing, the routine computes the
native maximum and compares it with live health. It writes the maximum
only when live health is strictly greater. It then advances past the
creature if health is zero or below. The maximum calculation and the
clamp use the same fields on this first turn as on subsequent turns.
Evidence at S 0x137856–0x13789b and C 0x211a96–0x211adb.

For the untouched bomb, the maximum uses its signed level, tier zero,
health multiplier 100, class zero and empty equipment. For the level
exponent, the routine subtracts 1 from the level, multiplies by 0.05 and
adds 1 to obtain the denominator. It divides 1 by that denominator,
subtracts the quotient from 1 and multiplies by 3. These steps use
binary32. It raises 2 to that exponent in binary64 and rounds the
power to binary32. The tier-zero power of 1 is multiplied by the
level power, then by 100, in binary32. Evidence at S 0xfda0–0xffce,
0x7164, 0x71d5, 0x7261 and C 0x44db0–0x44fde, 0x3c244, 0x3c2b5,
0x3c341.

For example, the level-1 maximum is 100.0 and the level-0 maximum is
about 89.6332. Level 70 gives a maximum above 500, so its fresh health
stays at 500.0 through the upper clamp. Under round-to-nearest with
masked floating-point exceptions, level −19 gives a zero denominator
and maximum zero, so the clamp reaches zero before the burn. Level −20
gives a maximum above 500.

With positive health after the clamp, the authority type-144 path takes
the elapsed integer milliseconds as binary32, multiplies by the
binary32 0.025, subtracts the product from live health in binary32 and
floors negative results to zero. A level-1 bomb with a 16 ms first turn
goes from 500.0 to 100.0 at the clamp and to about 99.6 after the
subtraction. The clamp and fuse subtraction use zero random draws.
Evidence at S 0x1325fd–0x132612, 0x13793b–0x137987, 0x173c8c and
C 0x20c83d–0x20c852, 0x211b7b–0x211bc7, 0x2fd564.

Malformed receipt, allocation failure, signed ID exhaustion, independent
changes to bomb health before its first turn, online client receipt,
later explosion behavior and runtime acceptance remain outside this
ruling.
