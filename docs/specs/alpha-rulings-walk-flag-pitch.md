Status: FINAL / STABLE / HELD

# Alpha rulings: walk-byte flag clause and head-pitch decay

Scope: two bounded rulings settling conflicts between held contracts
`alpha-travel.md`, `alpha-controller-aim-writers.md` and
`alpha-combat-aim-triplet.md`. Supported image: original `Cube.exe` (image base
0x400000). All addresses are RVAs unless written as VA. Evidence is the original
image's instruction bytes and constant data; a reconstruction listing was used only
to locate addresses and was confirmed against the image bytes. This document does
not edit the existing contracts; it states which of their statements hold.

Terms: C is the local controller, P0 the local player's creature (pointer at
C+`0x8006D0`), flags word P0+`0x124`, action byte P0+`0x68`, action clock P0+`0x6C`
(signed 32-bit milliseconds), head pitch P0+`0x58` (binary32), step dt (signed
32-bit milliseconds of the world update). Flag writes go through the bit setter
`0x02F160`, which sets or clears one mask in the flags word.

## 1. Ruling 1: the clause at `0x098318`

### 1.1 Behaviour

The clause is step 3 of the flags block `0x0982BE–0x098404` inside the local
controller update `0x088EE0`. It immediately follows the `0x40` write of
`0x0982D0–0x098313`.

1. **Byte and sense.** The clause tests controller input byte C+`0x14`. If the byte
   is **nonzero (held), the whole clause is skipped** and control goes directly to
   the `0x01` write at `0x0983F4`. The clause body runs only when the byte is **zero
   (not held)**. The physical key of C+`0x14` is Left Shift (keyboard scan code
   `0x2A`) according to the sampler table of `alpha-controller-aim-writers.md` §2;
   that table was not re-derived in this ruling.
2. **Excluded actions.** If the action byte is any of `0x6D`, `0x53`, `0x6A`, `0x6B`,
   `0x69`, `0x52`, `0x22`, `0x54`, `0x50`, `0x51`, `0x4F`, `0x0B` (decimal 109, 83,
   106, 107, 105, 82, 34, 84, 80, 81, 79, 11), the clause is skipped.
3. **Duration source.** R0 is the action-duration query `0x03D1A0` evaluated on P0
   (P0 is its only input; it dispatches on P0's action byte). The value R0 + 200 is
   formed as a 32-bit integer addition.
4. **Comparison.** The clock is compared with R0 + 200 as **signed 32-bit integers,
   strictly less than** (`0x098393–0x098396`). If clock < R0 + 200 the body is
   entered.
5. **Sustained actions.** Otherwise (clock ≥ R0 + 200) the body is entered only if
   the action byte is one of `0x18`, `0x19`, `0x1B`, `0x08`, `0x24`, `0x40`, `0x3B`,
   `0x3F`, `0x1C`, `0x23` (decimal 24, 25, 27, 8, 36, 64, 59, 63, 28, 35); for any
   other action the clause is skipped.
6. **Body.** If the action byte is `0x58`, `0x57`, `0x59` or `0x5C` (decimal 88, 87,
   89, 92), flag `0x04` is not touched (it keeps the value 0 written at `0x0982BE`).
   Otherwise flag `0x04` is **set**. In both cases flag `0x40` is then **cleared**
   (`0x0983E2–0x0983EF`).
7. **All other branches** (byte held, excluded action, or clock ≥ R0 + 200 with a
   non-sustained action) change neither `0x04` nor `0x40`: `0x04` stays cleared from
   step 1 of the block, and `0x40` keeps the value written by step 2.

Branch order is: byte test, exclusion list, R0 query, clock comparison, sustained
list, the four-action test, set `0x04`, clear `0x40`. No random draw occurs.

### 1.2 Gliding in the air

Nothing in the clause or in the block's entry tests flag `0x10` or the ground state.
The only glider-dependent part of the block is the preceding `0x40` write (which, when
`0x10` is set and bit 0 of P0 byte `+0x5C` is clear, uses jump byte C+`0x12` and the
stamina test). The clause therefore runs while gliding in the air exactly as on the
ground, and when its body runs it clears `0x40`, overriding the glider-lift value
just written. Consequence: while gliding with Left Shift not held, lift is cancelled
during the first R0 + 200 ms of any non-excluded action's clock (for action 0,
R0 = 100 per `alpha-controller-aim-writers.md` §4, so the first 300 ms), and
throughout the listed sustained actions. Holding Left Shift does not cancel lift.

The block itself is reached on the free-control path (byte VA `0x0076B040` zero, test
at `0x098078`). Earlier tests at `0x098049–0x098072` skip the whole flags block when
the object at C+`0x800A14` is non-null with nonzero byte `+0x180`, or when P0's
signed `+0x12C` is positive; none of these depends on flag `0x10`.

### 1.3 Which contract is right

- `alpha-controller-aim-writers.md` §4 step 3 is **correct**: "If input +0x14 is
  clear, … either the clock is below R0 + 200 … or the action is one of …".
- `alpha-travel.md` §2.2 step 3 is **wrong** on two points: "If the walk byte is held"
  (the body runs when the byte is not held) and "at least the action's total duration
  … + 200 ms" (the body runs when the clock is strictly below R0 + 200). Its action
  lists and its "0x04 := 1 unless the action is 87, 88, 89 or 92, and in either case
  0x40 := 0" match the image. Its closing sentence "after 300 ms of action time the
  walk byte also cancels the glider lift" is wrong; for action 0 the lift is
  cancelled only during the first 300 ms, and only when the byte is not held.
- `alpha-travel.md` correction 4 ("holding the walk key … sets 0x04 and clears 0x40
  even while gliding") is wrong in its sense; its "even while gliding" part is right.
- `alpha-travel.md` §2.2 step 2 ("Otherwise 0x40 := (walk byte not held)") agrees
  with the image (`0x098303–0x09830A`).

## 2. Ruling 2: head-pitch decay and the aim blend

### 2.1 Location and scope

The decay is inside the world update `0x20C510`, in its per-creature loop over the
world creature map (creature pointer fetched at `0x2104DB–0x2104DE`). The loop skips a
creature only when its entry is null (`0x2104E6`) or when its binary32 at `+0x16C`
is not greater than 0.0 (`0x211AD1–0x211ADB`; an unordered value does not skip).
No other jump from the loop body before `0x21F000` leaves past the decay. The decay
is therefore applied to **every creature processed by this loop**, not only the local
player; it is not tested against the local player identity.

### 2.2 Order within one creature's step

At `0x21F0F1–0x21F10F` a three-way test selects one of two exclusive paths:

1. **Climb-hold path** (flag `0x01` set, and bit `0x04` of byte `+0x5C` set, and flag
   `0x10` clear): head pitch is blended toward 45.0 at rate 0.005 and the wall yaw is
   applied, as in `alpha-travel.md` §6.4. This path then jumps to `0x21F62B`, so it
   applies **neither** the decay **nor** the face-aim block of
   `alpha-combat-aim-triplet.md` §3.7 **nor** the movement-yaw rule of that section.
2. **Otherwise** (`0x21F262` onward), in this order:
   1. the exp decay (2.3), every step, regardless of the face-aim bit;
   2. then the face-aim test at `0x21F29A`: if flag `0x04` is set and signed
      `+0x12C` ≤ 0, the §3.7 yaw and pitch blend runs; its pitch blend reads and
      blends the **already-decayed** pitch (both the angle-interpolation start value
      and the blended field are `+0x58` after the decay store). Otherwise the
      movement-yaw rule of §3.7 runs and pitch is not touched further here.

So, while the face-aim bit is set, each step first multiplies pitch by the decay
factor and then blends the result toward the aim tilt at rate 0.005.

### 2.3 Arithmetic

1. dt_f = the step converted from signed 32-bit integer to binary32 (`0x20C83D–0x20C84A`,
   computed once per world update).
2. x = dt_f × (−0.01) in binary32, where −0.01 is the binary32 constant `0xBC23D70A`
   at VA `0x0071E210` (`0x21F262–0x21F265`).
3. f = exp(x) through helper `0x222D40`: x is widened to binary64, the precise
   binary64 exponential import is applied, and the result is rounded to binary32.
4. Pitch := f × pitch, a single binary32 multiplication, stored to `+0x58`
   (`0x21F287–0x21F296`). No clamp and no random draw.

### 2.4 Which statements are right

- `alpha-travel.md` §6.4 "Otherwise the head pitch is multiplied by exp(−0.01·dt_f)
  every step (RVA 0x21f262)" is **correct**, with these precisions: "otherwise" means
  "unless the climb-hold path of 2.2 is taken"; it applies to every updated creature;
  and the constants and rounding are those of 2.3.
- `alpha-combat-aim-triplet.md` §3.7 ("Head pitch +0x58 is blended over the step at
  rate 0.005 toward …") is **correct** and does not conflict: the two operations are
  sequential in the same step, decay first. Its gate "the face-aim bit is set and
  signed +0x12C ≤ 0" is correct, with the further condition that the climb-hold path
  was not taken.
- The two contracts are not inverses on this point; neither replaces the other.

## 3. Unresolved

- The physical key of C+`0x14` (Left Shift) is taken from the held sampler table and
  was not re-derived here; `alpha-travel.md` calls it "the walk key" by reference to
  `alpha-player-control.md`, which was not read.
- R0's per-action values (query `0x03D1A0` and its speed-factor dependency) were not
  re-audited beyond confirming its input is P0; the action-0 value 100 is taken from
  `alpha-controller-aim-writers.md` §4.
- The full gating of the flags block (the role of C+`0x800A14` byte `+0x180`, the
  build-mode and other branches before `0x098049`) was not closed.
- The meaning of P0 byte `+0x5C` bits 0 and 2 (named on-ground and wall contact in
  `alpha-travel.md`) was not re-verified.
- Whether the creature map iterated by the world update holds remote creatures on the
  client (so that the decay also runs locally for them), and the corresponding
  routine in `Server.exe`, were not examined.
- The precise import's last-bit behaviour (`libm_sse2_exp_precise`, MSVCR110) is an
  external dependency and is not specified here.
