Status: FINAL / STABLE / HELD

# Alpha pose selectors 1 through 0x18: per-selector evaluator case bodies

This independent source-side contract closes the case bodies that the held contract `alpha-weapon-attachment-poses.md` leaves open (its Section 4) for pose selectors 1 through `0x18`. For each selector it states the decision order, the owner and subobject fields and helper results read, every constant, and every write to the pose target locals, the two rest hand-position locals, the blend-rate local and the selector field. It also records writes the held contract does not list (an owner field, the two weapon model pointers, a flag byte and three further locals) so that the implementer can bound them. Storage names follow the held contract; no gameplay meaning is asserted.

## Scope and evidence

- Supported image: client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base `0x00400000`. Hash verified before use. Server image `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, also verified (Section 9).
- All addresses are RVAs. Instructions were read from `decompiled/client/assembly.txt` (case bodies `0x013AFD`–`0x01E660`, the pre-evaluator code `0x012935`–`0x013AFC` and the post-evaluator code from `0x01E660`) and from `decompiled/client/assembly-functions/004128f0.asm` (later uses of locals). The dispatch table, every memory-resident binary32/binary64 constant and the imported routine names (through the import thunks at images `0x2FC444` and `0x2FC45C`) were read from the image bytes through the section table. Stack-slot identity was established by tracking the stack depth at every instruction of each case (pushes, pops, explicit adjustments and the callee stack-cleanup sizes read from each callee's return instruction) and by matching the slots to the post-evaluator blend code (RVAs `0x01E9D4`–`0x01F0A0`), which names the subobject field each slot is blended into.
- Decompiler output was not used. The Infinite-Cubes reconstruction was not consulted. The held contracts `alpha-weapon-attachment-poses.md` and `alpha-combat-helper-tables.md` were used only to locate the evaluator, its storage terms and the timing helpers; every fact below was read again from the original.

## 1. Terms

### 1.1 Owner, subobject, helpers

- **Owner**: the creature whose pose subobject C is being refreshed (C's first word). "Owner `+x`" is a creature field, "C `+x`" a subobject field, as in the held contract.
- **c**: the signed 32-bit pose clock C `+0x4`, read fresh at each use. **t**: c converted to binary32 by the packed integer conversion (exact up to magnitude 2^24, otherwise rounded under the ambient MXCSR mode). "binary32 of an integer expression" means the same conversion of the 32-bit result.
- **B**, **Cq**, **R0**: the timing helpers RVA `0x03CAA0`, RVA `0x047310` and RVA `0x03D1A0` of `alpha-combat-helper-tables.md` (its sections 3, 4 and 5), each called on the owner; B and Cq are always called here with the argument −1 (meaning the owner's current action byte). Each mention below is one call; the helpers are pure, so repeated calls return the same value.
- **P**: the predicate RVA `0x044230` on the owner (helper contract section 6.1), true or false.
- **half(x)**: signed 32-bit x divided by 2 with truncation toward zero. **2x**: 32-bit wrapping doubling. Integer subtractions such as c − B and c − 500 are 32-bit wrapping. All comparisons of c with integers or helper results are signed 32-bit.
- **cosw(x)**, **sinw(x)**: the wrappers RVA `0x00E420` and RVA `0x024B50`. Each takes a binary32 argument, widens it exactly to binary64, calls the imported MSVCR110 `_libm_sse2_cos_precise` (wrapper `0x00E420`, thunk `0x28D7A4`) or `_libm_sse2_sin_precise` (wrapper `0x024B50`, thunk `0x28D9C2`), rounds the result to binary32 and returns that binary32 value unchanged. Where a case calls an import directly, the conversions are stated at that case.
- **Model lookup**: RVA `0x0120C0`, applied to the model collection (the refresh's fourth argument, held in a register since RVA `0x012935` and not modified before or inside the cases) with index 840 (`0x348`). It returns the 32-bit entry at that index when 0 ≤ index < the collection's element count (the difference of the collection words `+0x8` and `+0x4`, divided by 4), otherwise null. "Entry 840" below means this result.

### 1.2 Local storage written by the cases

All triplets are three binary32 components written X, Y, Z (component 0, 1, 2). The labels are used only in this contract.

| Label | Held-contract storage | Consumer after the evaluator |
|---|---|---|
| MA | main-hand angle target | blended into C `+0x30` |
| OA | off-hand angle target | blended into C `+0x24` |
| MO | main-hand offset target | blended into C `+0x18` |
| OO | off-hand offset target | blended into C `+0x0C` |
| BA | body angle target | blended into C `+0x54` |
| CA | creature angle target | approached into C `+0x60` by the angle-approach helper |
| HA | head angle target | approached into C `+0x84` by the angle-approach helper |
| CO | creature offset target | blended into C `+0xAC` |
| BO | body offset target | blended into C `+0x6C` |
| HT | hand-turn target | blended into C `+0x9C` |
| MP | main-hand rest hand position | receives += C `+0x18` at RVAs `0x01F085`–`0x01F090` |
| OP | off-hand rest hand position | receives += C `+0x0C` at RVAs `0x01F095`–`0x01F0A0` |
| RATE | blend rate (binary32) | the rate of the blend amount approach (held contract Section 5) |
| S | selector C `+0x8` (signed 32-bit) | post-evaluator code |

The lift target, the turn-scalar target, the target blended into C `+0x78`, the tail angle target and both rest hand-angle locals are not written by any of these cases.

Further locals, not named by the held contract:

- **OM** (off-hand mid-point local) and **MM** (main-hand mid-point local). Before the evaluator (RVAs `0x013743`–`0x01381C`), with h0 = owner `+0xF4` and h2 = owner `+0xFC` and the final rest positions OP and MP of held Section 3: OM.X = ((h0 × −0.3) + OP.X) × 0.5; OM.Y = ((OP.Y − 8) × 0.5) × 0.5; OM.Z = (OP.Z + (h2 + 12)) × 0.5; MM.X = (MP.X + (h0 × 0.3)) × 0.5; MM.Y = binary32 of ((binary64 of ((MP.Y − 8) × 0.5)) × binary64 0.5); MM.Z = (MP.Z + (h2 + 12)) × 0.5. Constants: −0.3 `0xBE99999A` (image `0x2FCE88`), 0.3 `0x3E99999A` (`0x2FCDC0`), 8 `0x41000000` (`0x345E70`), 12 `0x41400000` (`0x2FCE30`), binary32 0.5 `0x3F000000` (`0x345DA0`), binary64 0.5 `0x3FE0000000000000` (`0x345DE8`). After the evaluator (RVAs `0x01E66D`–`0x01E6EC`), before the corrections, OM becomes (OP + OM) × 0.5 and MM becomes (MP + MM) × 0.5 component-wise, using OP and MP as left by the case. Later reads of OM and MM (from RVA `0x01F0BE`) are outside this contract. Only OM is written by these cases.
- **FA** and **FB**: two triplet locals filled before the evaluator, FA = (owner `+0x100`, `+0x104`, `+0x108`) and FB = (the same with X sign-flipped) (RVAs `0x013825`–`0x01388D`). Their later reads (from RVA `0x023971`) are outside this contract.
- **FLAG**: a byte local cleared to 0 before the evaluator (RVA `0x013941`) and tested at RVA `0x023A1C`; its effect is outside this contract.
- Every other stack slot a case writes is a scratch value or a constructor temporary that is written before it is read inside the case; none was found to be read by the post-evaluator code before being rewritten.

### 1.3 Conventions for the case descriptions

- "Set V = (x, y, z)" writes all three components; "V.Y = k" writes one component; "V.Y += k" and "V.Y −= k" read the component's value at that point (the pre-case value of held Section 3 unless the case changed it earlier) and store one binary32 sum or difference. Every constant stored by a set is the exact bit pattern of the value shown (immediates `0xC2B40000` = −90, `0x42B40000` = 90, and so on). Copies ("OP = MP") copy three 32-bit words.
- Arithmetic is scalar binary32 with one rounding per operation, in the order written; there are no fused operations. "a × b + c" is written with explicit parentheses where it matters.
- A clamp "if x > 1 then x = 1" is an ordered comparison: a NaN x is kept. Where both bounds are clamped this is stated.
- Unless stated otherwise every path ends by continuing at the post-evaluator code RVA `0x01E660`. No case returns from the routine early, and no case writes the pose clock C `+0x4` (the post-evaluator code zeroes it when S is zero, held Section 5).
- RATE is 0.01 (`0x3C23D70A`) at case entry (RVA `0x013946`). "R = k" means the case stores k; on paths that store nothing the effective rate stays 0.01.

### 1.4 Binary32 constants read from memory

| Value | Bits | Image | Value | Bits | Image |
|---|---|---|---|---|---|
| 0.005 | `0x3BA3D70A` | `0x2FCD9C` | 14 | `0x41600000` | `0x2FCE34` |
| 0.01 | `0x3C23D70A` | `0x2FCDA0` | 15 | `0x41700000` | `0x2FCE38` |
| 0.02 | `0x3CA3D70A` | `0x2FCDA8` | 16 | `0x41800000` | `0x345E78` |
| 0.03 | `0x3CF5C28F` | `0x2FCDAC` | 20 | `0x41A00000` | `0x2FCD74` |
| 0.04 | `0x3D23D70A` | `0x2FCDB0` | 30 | `0x41F00000` | `0x2FCD78` |
| 0.05 | `0x3D4CCCCD` | `0x2FCDB4` | 60 | `0x42700000` | `0x2FCE48` |
| 0.1 | `0x3DCCCCCD` | `0x2FCB7C` | 70 | `0x428C0000` | `0x2FCE4C` |
| 0.5 | `0x3F000000` | `0x345DA0` | 90 | `0x42B40000` | `0x2FCE54` |
| 1 | `0x3F800000` | `0x345DC0` | 120 | `0x42F00000` | `0x2FCE58` |
| 1.5 | `0x3FC00000` | `0x2FCDE0` | 300 | `0x43960000` | `0x2FCE68` |
| 2 | `0x40000000` | `0x345E10` | 360 | `0x43B40000` | `0x2FCE6C` |
| 3 | `0x40400000` | `0x2FCD70` | 500 | `0x43FA0000` | `0x2FCE70` |
| 4 | `0x40800000` | `0x345E60` | −20 | `0xC1A00000` | `0x2FCE9C` |
| 5 | `0x40A00000` | `0x2FCE24` | −40 | `0xC2200000` | `0x2FCEA0` |
| 6 | `0x40C00000` | `0x345E64` | −50 | `0xC2480000` | `0x2FCEA4` |
| 7 | `0x40E00000` | `0x2FCE28` | −60 | `0xC2700000` | `0x2FCEA8` |
| 8 | `0x41000000` | `0x345E70` | −360 | `0xC3B40000` | `0x2FCEBC` |
| 10 | `0x41200000` | `0x345E74` | +0.0 | `0x00000000` | `0x345D84` |

Binary64 constants: pi `0x400921FB54442D18` (image `0x2FCE08`), pi/2 `0x3FF921FB54442D18` (`0x2FCDF0`), 0.5 `0x3FE0000000000000` (`0x345DE8`). Sign flips of a value use an exclusive-or with the sign mask at image `0x345F70` (exact).

## 2. Dispatch

At RVA `0x013AE9` the selector S is read, decremented and compared unsigned with `0x48`; values above go to `0x01E660`, otherwise the 32-bit entry (S − 1) of the table at image `0x023D4C` is the jump target. Entries for selectors 1 through `0x18`:

| S | Case RVA | S | Case RVA | S | Case RVA |
|---|---|---|---|---|---|
| 1 | `0x0174A0` | 8 | `0x019EBD` | `0x12` | `0x01D123` |
| 2 | `0x01644B` | 9 | `0x01B54E` | `0x13` | `0x01BE27` |
| 3 | `0x0170EE` | `0x0A` | `0x01B850` | `0x14` | `0x01BADC` |
| 4 | `0x016052` | `0x0B` | `0x01B006` | `0x15` | `0x018591` |
| 5 | `0x019A6F` | `0x0F` | `0x01DB88` | `0x16` | `0x017C84` |
| 6 | `0x019BEE` | `0x10` | `0x01E029` | `0x17` | `0x01A5C1` |
| 7 | `0x017AB1` | `0x11` | `0x01D3EE` | `0x18` | `0x01AD90` |

The entries for `0x0C`, `0x0D` and `0x0E` hold `0x01E660`: those selectors go straight to the post-evaluator code with no case write. (The table's first case body starts at `0x013AFD`, one byte after the end of the indirect jump.)

## 3. Shared sub-blocks

Several cases finish in common code. Each block is described once.

- **Assign tails.** The code at RVA `0x01E65A` copies a triplet (prepared by the jumping case) into a target and continues at `0x01E660`. The code at RVA `0x01E64E` builds a triplet from three constants supplied by the jumping case and copies it into MA. In the case descriptions these simply appear as the last "set".
- **Block G** (RVA `0x016DDD`, reached from selector 3): OP = MP; OP.X += 2; OM.Y += 10; OM.Z −= 4; R = 0.005; OA = MA.
- **Block H** (RVA `0x016885`, reached from selectors 2 and 3): OA = MA (performed twice; idempotent).
- **Block R** (two-hand grip; inline in selectors 2, 8, `0x0F`, `0x10` and inside block T): MP.Y += 2; MP.Z += 3; OP = MP; OP.X −= 3; OP.Z −= 3; OM.Y += 10; OM.Z −= 4; MA.X = 0; MA.Y = 20; then OA = MA. MA.Z is not written.
- **Block T** (RVAs `0x0195C3`–`0x019743`, inside the selector `0x1D` body, reached from selectors `0x0A`, `0x11`, `0x12`). Entry at `0x0195C3` or `0x0195C6`: set OA = (−80, 40, 60). Entry at `0x0195DD`: set OA to the triplet prepared by the jumping case. Then OO.Y −= 4; OO.Z += 2. Then, if the main-hand model pointer C `+0xDC` is null, or the off-hand model pointer C `+0xE0` is nonnull, the block ends. Otherwise block R runs (ending with OA = MA, which overwrites the OA just set).
- **Block Q** (the "late" branch shared in form by selectors `0x0A`, `0x0F`, `0x10`, `0x11`, `0x12`; each copy is inline): set HT = (0, 0, 0); S = 0; read the owner signed 32-bit type value `+0x64`; if it equals `0x60` or `0x57` the branch stops here (continuing as stated per selector). Otherwise OA.X = q (q = −90, or −60 for selector `0x10`; always overwritten later in the branch by a full OA set); BA.Z = 20; set MA = (−120, −90, 30); then the selector's own OA choice follows.
- **Cosine half-add** (RVA `0x01A5A9`): given a cosine wrapper result k for a target component V, V = (k × 0.5) + V. Reached from selector `0x17`.
- **Sine add** (RVA `0x01BE09`): given a sine wrapper result k for a component V, V = k + V, then R = 0.03. Reached from selectors `0x0A` (no halving) and, after a × 0.5, from selectors `0x13` and `0x14`.

## 4. Selectors 1 to 4

### Selector 1 (RVA `0x0174A0`–`0x017AB0`)

1. **Early phase, t < 500** (ordered binary32 comparison). r = t ÷ 500; if r > 1 then r = 1 (no lower clamp).
   - If P: MA.X = r × 120; MO.Y = 0; MO.Z = 14; MO.X = the sign-flipped MP.X; OO.Y = 0; OO.Z = 12; OO.X = 2 − OO.X.
   - Otherwise: set MO = (1, 0, 14); set OO = (−1, 0, 14); OA.X = −100; MA.X = −100.
   - Then, for both: BA.X = r × −40 and CA.X = the same value r × −40. (The product r × −50 is also formed and stored only to a scratch local that nothing reads.) If the owner action byte `+0x68` equals `0x30`: u = (t ÷ 500) × 360, if u > 360 then u = 360, and CA.X = (r × −40) − u.
   - Then the owner binary32 field `+0x1188` is set to +0.0 (32-bit zero store), whether or not the action is `0x30`. This is an owner write: `+0x1188` is the W input of held Section 3, so it affects the arm swing and rest positions of later refreshes.
   - R is not written (effective 0.01).
2. **Otherwise, c < R0** (R0 evaluated once). u = binary32 of (c − 500) ÷ 500, then × 2; if u > 1 then u = 1. CA.X = (1 − u) × −60; BA.X = −20; MA.X = −70.
   - If P: MO.Y = 4; MO.Z = 0; MO.X = the sign-flipped MP.X; OO.Y = 4; OO.Z = 0; OO.X = 2 − OO.X; R = 0.1.
   - Otherwise: set MO = (1, 3, 8); set OO = (−1, 3, 8); OA.X = −150; MA.X = −150; R = 0.1.
3. **Otherwise (c ≥ R0)**.
   - If P: MP.Y += 4; MP.X −= 8; OP = MP; OP.X −= 2; OP.Z −= 1; OM.Y += 10; OM.Z −= 4; MA.X = −60; MA.Z = 20.
   - Otherwise: set MO = (1, 3, 2); set OO = (−1, 3, 2); OA.X = −250; MA.X = −250.
   - R is not written.
4. **Tail, all paths**: if the owner off-hand kind byte `+0x990` equals 3 and the off-hand subtype byte `+0x991` equals `0x0D`, set OA = (−250, −90, 0).

P is evaluated at most once per path. S is not written.

### Selector 2 (RVA `0x01644B`–`0x0168AB`)

1. **c < B**. a = (t ÷ binary32 of B) × 1.5, with a fresh B call for the divisor; if a > 1 then a = 1. b = 1 − a.
   - CA.X = (a × 10) + 5.
   - Set BA = (10, 10, −60).
   - Set MA = ((b × 90) + (a × 60), (b × +0.0) + (a × +0.0), (b × 90) + (a × 60)). The +0.0 factor is a zeroed register; the Y component is a signed zero for finite a and b and NaN otherwise.
   - Set MO = (0, 0, a × 14); OO = MO.
   - OP = MP; OP.X += 2; OM.Y += 10; OM.Z −= 4; R = 0.01 (stored, same as the entry value); OA = MA.
2. **Otherwise, c < R0**. f = binary32 of (c − B) ÷ binary32 of Cq; if f > 1 then f = 1 (no lower clamp).
   - R = 0.04. CA.X = −10; BA.Y = 10; BA.Z = 60 (BA.X is not written). Set MA = (−70, 0, −60).
   - The sine argument is formed in binary64 as ((binary64 of f) × pi) × 0.5, each product rounded once, then rounded to binary32 and widened back to binary64. g is the imported sine (called directly) of that value, rounded to binary32, and k = g × 2 in binary32.
   - Set MO = (0, k, 0); OO = MO. OP = MP; OP.X += 2; OP.Y −= 1; then block H (OA = MA).
3. **Otherwise (c ≥ R0)**. S = 0. Block R (MP.Y += 2; MP.Z += 3; OP = MP; OP.X −= 3; OP.Z −= 3; MA.X = 0; MA.Y = 20; OM.Y += 10; OM.Z −= 4; OA = MA). R is not written.

### Selector 3 (RVA `0x0170EE`–`0x01749F`)

1. **c < B**: CA.Z = 0; set MA = (−90, 0, 0); set MO = (−5, 8, 0); OO = MO; then block G.
2. **Otherwise, c < B + half(Cq)** (Cq called first, then B; the sum is 32-bit wrapping): set CA = (10, 0, −10); set BA = (30, 0, −30); set MA = (0, 0, 90); set MO = (−8, −4, 20); OO = MO; then block G.
3. **Otherwise, c < R0**: CA.Z = −30; set BA = (0, 20, −60); set MA = (70, 0, 90); set MO = (5, −2, 0); OO = MO; OP = MP; OP.X += 2; OM.Y += 10; OM.Z −= 4; R = 0.02; OA = MA.
4. **Otherwise**: S = 0; OP = MP; OP.X −= 2; OP.Z −= 1; OM.Y += 10; OM.Z −= 4; MA.Y = 90; BA.Z = −80; then block H (OA = MA). R is not written.

Paths 1 and 2 end in block G, so R = 0.005 there.

### Selector 4 (RVA `0x016052`–`0x01644A`)

1. **c < half(B)**. a = t ÷ binary32 of half(B) (fresh B call); if a > 1 then a = 1. b = 1 − a.
   - CA.X = a × 5; BA.Y = a × 10; BA.Z = a × −20 (BA.X is not written).
   - Set MA = ((b × +0.0) + (a × 60), (b × 120) + (a × 60), (b × +0.0) + (a × 60)); the +0.0 factor is the memory constant at image `0x345D84`.
   - OP = MP; OP.X += 2; OM.Y += 10; OM.Z −= 4; R = 0.005; OA = MA.
2. **Otherwise, if owner byte `+0x140` equals 1 and owner byte `+0x141` equals 1**: set MO = (2, 7, 4); set MA = (0, −60, 0); then the common end with R = 0.01.
3. **Otherwise**: e = (binary32 of (c − half(B)) ÷ binary32 of half(B)) × 1.5 (two B calls); if e > 1 then e = 1. f = 1 − e.
   - CA.X = 0; set BA = (20, 0, 0).
   - Set MA = ((f × 90) + (e × 30), (f × +0.0) + (e × +0.0), (f × 90) + (e × +0.0)) (here +0.0 is a zeroed register).
   - MP.Z = (k × 0.5) + MP.Z, where k is the binary32 rounding of the imported cosine (called directly) of binary64 of (t × 0.5).
   - Set MO = (−6, 0, e × 16); then the common end with R = 0.005.
4. **Common end of paths 2 and 3**: R as stated; OO = MO; OP = MP (after any MP change); OP.X += 2; OM.Y += 10; OM.Z −= 4; OA = MA.

S is not written by selector 4.

## 5. Selectors 5 to 0x0B

### Selector 5 (RVA `0x019A6F`–`0x019BED`)

No condition. Set MA = (−90, 0, −40); set OA = (−90, 0, 40). MP.Z = (cosw(t × 0.5) × 0.5) + MP.Z; then OP.Z = (cosw(t × 0.5) × 0.5) + OP.Z (a second, identical evaluation). Set MO = (1, −7, 0); set OO = (−1, −7, 0); BA.Z = −40; R = 0.005.

### Selector 6 (RVA `0x019BEE`–`0x019EBC`)

1. **If owner byte `+0x141` equals 1**: set MA = (−50, −60, 40); set OA = (−90, 90, −50); the two cosine Z additions of selector 5 (MP.Z, then OP.Z); set MO = (−3, 7, 3); set OO = (−2, 2, 0); BA.Z = −20; R = 0.005.
2. **Otherwise**: set MA = (−90, 0, −40); set OA = (−90, 90, −40); the same two cosine Z additions; set MO = (1, −7, 0); set OO = (−1, 0, 1) (built at RVA `0x019BAE` inside selector 5's body); BA.Z = −40; R = 0.005.

### Selector 7 (RVA `0x017AB1`–`0x017C83`)

1. **c < half(R0)**: set HT = owner triplet `+0x154`, `+0x158`, `+0x15C` (binary32); R = 0.005.
2. **Otherwise, c < R0**: m = binary32 of (c − half(R0)) ÷ binary32 of half(R0); the comparison with R0, the subtraction and the divisor each use a fresh R0 call, in that order. k = ((m × m) × m) × 15; if k > 1 then k = 1 (no lower clamp). Set HT component-wise to (owner `+0x148` triplet × k) + (owner `+0x154` triplet × (1 − k)), each product and the sum rounded once. R = 0.03.
3. **Otherwise**: set HT = owner triplet `+0x148`, `+0x14C`, `+0x150`. R is not written.
4. **All paths**: set MA = (0, 90, 0). If P: OP = MP; OP.X += 2; OM.Y += 10; OM.Z −= 4.

### Selector 8 (RVA `0x019EBD`–`0x01A2FC`)

The clock is compared directly as a signed integer with 100 and 500. The first test is the off-hand model pointer C `+0xE0`.

1. **C `+0xE0` nonnull**:
   - c < 100: BA.Z = 80; set OA = (0, 0, 90); set MA = (−45, 45, 0).
   - 100 ≤ c < 500: BA.Z = −90; set OA = (0, 60, 90); OO.Z += 5; set MA = (−90, 0, 45).
   - c ≥ 500: BA.Z = −30; set OA = (0, 100, 60); set MA = (−45, 45, 0).
2. **C `+0xE0` null**:
   - c < 100: BA.Z = −80; set MA = (0, 0, 90).
   - 100 ≤ c < 500: BA.Z = 90; set MA = (0, 60, −90); MA.Z += 5 (giving −85); OO.X += 2; MO.X += 2; OO.Y += 3; MO.Y += 3; OO.Z += 3; MO.Z += 3.
   - c ≥ 500: block R without its final copy (MP.Y += 2; MP.Z += 3; OP = MP; OP.X −= 3; OP.Z −= 3; OM.Y += 10; OM.Z −= 4; MA.X = 0; MA.Y = 20).
   - Then, for all three: OP = MP; OP.X −= 3; OP.Z −= 3; OA = MA. (For c ≥ 500 this recomputes the same OP.)

R and S are not written by selector 8.

### Selector 9 (RVA `0x01B54E`–`0x01B688`)

1. **c < B**: BA.Z = 80; set OA = (0, 0, 90); set MA = (−45, 45, 0). Then, if the owner action byte `+0x68` equals `0x30`: CA.Z = (n ÷ 300) × 360, where n is the binary32 conversion of the 32-bit wrapping negation of c.
2. **Otherwise**: BA.Z = −90; set OA = (0, 0, 90); set MA = (−90, 0, 45).

R and S are not written.

### Selector `0x0A` (RVA `0x01B850`–`0x01BADB`)

1. **c < B**: exactly the first path of selector 9 (including the action `0x30` creature-angle write).
2. **Otherwise, c < R0**: BA.Z = −90; set OA = (0, 0, 90); set MA = (−90, 0, 45); then OP.X = sinw(w × 0.1) + OP.X, where w is the binary32 conversion of the 32-bit wrapping difference c − 600 (no halving), and R = 0.03 (the sine add block).
3. **Otherwise**: block Q with q = −90; if the type value test stops the branch, continue at `0x01E660`. Otherwise OA is chosen: if the owner off-hand subtype `+0x991` equals `0x0D`, block T is entered at `0x0195DD` with OA = (0, 0, 90); otherwise at `0x0195C6` with OA = (−80, 40, 60). Block T then applies OO.Y −= 4, OO.Z += 2 and, under its model-pointer condition, block R (which replaces OA by MA). R is not written on this path.

### Selector `0x0B` (RVA `0x01B006`–`0x01B08A`)

If bit 2 (mask `0x04`) of the low byte of the owner 16-bit field `+0x7E` is set, nothing is written. Otherwise: BA.Z = 60; set MA = (0, −45, −60); set OA = (−45, −45, 0). R and S are not written.

## 6. Selectors 0x0F to 0x14

### Selector `0x0F` (RVA `0x01DB88`–`0x01E028`)

1. **Spin term.** v = owner signed 32-bit `+0x1314`; z = the signed remainder of half(v) divided by 3 (sign of the dividend). If z equals 2: f = binary32 of (c − B) ÷ binary32 of Cq, clamped to [0, 1] (if f > 1 then 1, else if 0 > f then 0; NaN is kept); CA.Z = f × −360; R = 0.03. Otherwise CA.Z = 0.
2. Set HT = owner triplet `+0x148`.
3. **c < B** (fresh B call): BA.Z = 40; set OA = (0, −90, 30); OO.Y −= 4; OO.Z += 4; set MA = (0, −90, −120); MO.Y −= 6.
4. **Otherwise, c < R0**: BA.Z = −90; CO.X = −0.1 (`0xBDCCCCCD`); set OA = (−90, −90, −30); set MA = (0, −90, −120); MO.Y −= 6.
5. **Otherwise**: block Q with q = −90; if not stopped: set OA = (−80, 40, 60); OO.Y −= 4; OO.Z += 2; then, only when C `+0xDC` is nonnull and C `+0xE0` is null, block R (ending with OA = MA). When block Q stops, the path goes directly to step 6.
6. **Tail, all of steps 3–5**: if owner `+0x991` equals `0x0D`, set OA = (0, 0, 90). Unlike selectors `0x0A` and `0x12`, this override is applied after block R.

R is 0.03 only when the spin term's remainder is 2; otherwise 0.01.

### Selector `0x10` (RVA `0x01E029`–`0x01E575`)

1. Set HT = owner triplet `+0x148`.
2. **Spin term** as in selector `0x0F` but taken when the remainder z equals 1 (same clamp; CA.Z = f × −360; R = 0.03); otherwise CA.Z = 0.
3. **c < B**: BA.Z = 90; set MA = (0, −90, 0); set OA = (−90, 0, 90); MO.Y += 6; MO.Z += 4.
4. **Otherwise, c < R0**: BA.Z = 20; set OA = (−90, −60, 90); OO.Y −= 4; OO.Z += 4; set MA = (0, −90, −220); MO.Y −= 6; CO.X = 0.1 (`0x3DCCCCCD`); FB.Y −= 4; FA.Y += 4; FLAG = 1.
5. **Otherwise**: block Q with q = −60; if not stopped: set OA = (−80, 40, 60); OO.Y −= 4; OO.Z += 2; block R under the same model-pointer condition as selector `0x0F`. When block Q stops, the path goes directly to step 6.
6. **Tail, steps 3–5**: if owner `+0x991` equals `0x0D`, set OA = (0, 0, 90).

### Selector `0x11` (RVA `0x01D3EE`–`0x01D62C`)

1. **c < B**: BA.Z = 40; set MA = (−90, 90, 30); set OA = (−90, 0, −20); OO.Y −= 6 (code shared with selector `0x12` at RVA `0x01D1B6`). R not written.
2. **Otherwise, c < R0**: BA.Z = −90; set MA = (−40, 0, 30); set MO = (3, 0, 4); set OA = (−90, 90, 90); set OO = (−3, −1, 0); FLAG = 1; R = 0.05 (store shared with selector `0x12` at RVA `0x01D2E7`).
3. **Otherwise**: block Q with q = −90; if the type value test stops the branch, continue at `0x01E660`; otherwise block T entered at `0x0195C3` (OA = (−80, 40, 60) with no subtype test), then its offset adjustments and conditional block R.

### Selector `0x12` (RVA `0x01D123`–`0x01D3ED`)

1. **c < B**: BA.Z = −40; set OA = (−90, −90, −30); set MA = (−90, 0, 20); MO.Y −= 6. R not written.
2. **Otherwise, c < R0**: BA.Z = 90; set OA = (0, 0, 40) when owner `+0x991` equals `0x0D`, otherwise (−40, 0, −30); set OO = (−3, 0, 4); set MA = (−90, −90, −90); set MO = (3, −1, 0); FLAG = 1; R = 0.05.
3. **Otherwise**: block Q with q = −90; if not stopped, block T entered at `0x0195DD` with OA = (0, 0, 90) when `+0x991` equals `0x0D`, otherwise at `0x0195C6` with OA = (−80, 40, 60).

### Selector `0x13` (RVA `0x01BE27`–`0x01C114`)

1. **c > B and c < R0** (R0 evaluated only when c > B): BA.Z = −20; CA.Z = −20; set OA = (0, 90, 40); set OO = (0, 8, 4); set MA = (0, 90, 30); set MO = (1, −4, −1); then (code at RVA `0x01BBEC` in selector `0x14`) HA.X = −10; BO.Y = 3; R = 0.03.
2. **Otherwise**: BA.Z = 0; CA.Z = 0; set MA = (60, −20, 20).
   - p = t × 0.01. Set MO = (0, sinw(p) + 2, cosw(p) + 4); cosw(p) is evaluated before sinw(p).
   - Set OA = (60, 20, −20).
   - p2 = binary32 of ((binary64 of (t × 0.01)) + pi/2), the sum in binary64. Set OO = (0, sinw(p2) + 2, cosw(p2) + 4), cosine first.
   - BO.Z = (sinw(t × 0.005) × 0.5) + BO.Z; R = 0.03.

### Selector `0x14` (RVA `0x01BADC`–`0x01BE26`)

1. **c > B and c < R0**: BA.Z = 45; CA.Z = 45; set MA = (0, −90, −90); set MO = (8, 0, 4); set OA = (0, −90, −30); set OO = (−1, −4, −1); HA.X = −10; BO.Y = 3; R = 0.03.
2. **Otherwise**: identical to the second path of selector `0x13`, except that the body-offset term uses t × 0.01: BO.Z = (sinw(t × 0.01) × 0.5) + BO.Z; R = 0.03.

S is not written by selectors `0x13` and `0x14`.

## 7. Selectors 0x15 to 0x18

### Selector `0x15` (RVA `0x018591`–`0x018AC5`)

The first test is the owner main-hand subtype byte `+0xAA9`. The case reads no stack argument of the refresh other than through the model collection register; in particular it does not read the refresh's tenth (mount) argument.

**Path M (main-hand subtype neither 6 nor 7):**

1. If owner `+0x991` equals 7: OA.Y = −90.
2. **c < 2R0**:
   - If owner `+0x991` equals 6: OO.Y += 5; OO.X −= 7. Otherwise OO.Y += 2; OO.X −= 3.
   - OO.Z += 4; MO.Y += 3; MO.X −= 10; MO.Z += 3; BA.Z −= 70; OA.Z = 70; MA.Z = 70.
   - **If also c < half(R0)**: OO.Z −= 2; OA.X = −5; set MO = (−5, 1, 0); the main-hand model pointer C `+0xDC` is set to null; BA.Z = −50; OA.Z = 70. The path then ends (the model step 4 below is skipped).
3. **c ≥ 2R0**: OO.X −= 1; OA.X = −30; MA.X = −30; MA.Y = 45; if owner `+0x991` equals 7, OA.Z = 50.
4. **Model step** (after step 3, or after step 2 when c ≥ half(R0)): if the owner off-hand kind byte `+0x990` equals 3, C `+0xDC` = entry 840.

**Path N (main-hand subtype 6 or 7):**

1. If the subtype is 7: MA.Y = −90.
2. **c < 2R0**:
   - If owner `+0xAA9` equals 6: MO.Y += 5; MO.X += 7. Otherwise MO.Y += 2; MO.X += 3.
   - MO.Z += 4; OO.Y += 3; OO.X += 10; OO.Z += 3; BA.Z = 70; MA.Z = −70; OA.Z = −70.
   - **If also c < half(R0)**: MO.Z −= 2; MA.X = −5; set OO = (5, 1, 0); the off-hand model pointer C `+0xE0` is set to null; BA.Z = 50; MA.Z = −70. The path then ends.
3. **c ≥ 2R0**: MO.X += 1; MA.X = −30; OA.X = −30; OA.Y = −45.
4. **Model step**: if the owner main-hand kind byte `+0xAA8` equals 3, C `+0xE0` = entry 840.

R and S are not written. The model pointer writes feed the held contract's Sections 8 and 9 (its part-mask clearing still follows).

### Selector `0x16` (RVA `0x017C84`–`0x017F9D`)

**Main-hand subtype `+0xAA9` neither 6 nor 7:** OO.Y += 3; OO.X −= 4; OO.Z += 2; MO.Y += 2; MO.X −= 10; MO.Z += 1; BA.Z = −60; OA.Z = 60; OA.Y = −90; MA.Z = 60. If owner `+0x990` equals 3, C `+0xDC` = entry 840. Then, if c < 300 (signed integer): OO.Z −= 1; OA.X += 20; OA.Y −= 20.

**Subtype 6 or 7:** MO.Y += 3; MO.X += 4; MO.Z += 2; OO.Y += 2; OO.X += 10; OO.Z += 1; BA.Z = 60; MA.Z = −60; MA.Y = −90; OA.Z = −60. If owner `+0xAA8` equals 3, C `+0xE0` = entry 840. Then, if c < 300: MO.Z −= 1; MA.X += 20; MA.Y += 20.

R and S are not written.

### Selector `0x17` (RVA `0x01A5C1`–`0x01A80A`)

1. **c < B**: BA.Z = −30; set MA = (0, −45, −60); set OA = (−45, −45, 0); MO.Y += 4; OO.Y += 4; then, in this order, MO.X = (sinw(t × 0.04) × 0.5) + MO.X; MO.Z = (cosw(t × 0.05) × 0.5) + MO.Z; OO.X = (sinw(t × 0.04) × 0.5) + OO.X; OO.Z = (cosw(t × 0.05) × 0.5) + OO.Z (the last through the cosine half-add block). Each wrapper call is a fresh evaluation.
2. **Otherwise**: BA.Z = 60; set MA = (0, −45, −60); set OA = (−45, −45, 0).

R and S are not written.

### Selector `0x18` (RVA `0x01AD90`–`0x01B005`)

1. **c < B**: MO.Y += 4; OO.Y += 4; the same four sine and cosine additions as selector `0x17` path 1, in the same order; BA.Z = 20.
2. **Otherwise, c < R0**: CO.X = 0.5; BA.Z = −60; CA.Z = −30; set OA = (0, 45, 60); set MA = (−45, 45, 0); set OO = (−2, 0, 0); FLAG = 1.
3. **Otherwise**: nothing is written.

R and S are not written.

## 8. Summary of side effects beyond the target locals

| Effect | Selectors (path) |
|---|---|
| S = 0 (the post-evaluator code then zeroes the clock; later selector tests in held Sections 7 and 10 see 0) | 2 (c ≥ R0), 3 (c ≥ R0), `0x0A`, `0x0F`, `0x10`, `0x11`, `0x12` (block Q paths) |
| Owner `+0x1188` = +0.0 | 1 (t < 500) |
| C `+0xDC` null | `0x15` (path M, c < half(R0)) |
| C `+0xDC` = entry 840 | `0x15` (path M model step), `0x16` (first form) |
| C `+0xE0` null | `0x15` (path N, c < half(R0)) |
| C `+0xE0` = entry 840 | `0x15` (path N model step), `0x16` (second form) |
| FLAG = 1 | `0x10`, `0x11`, `0x12` (middle paths), `0x18` (middle path) |
| FA.Y += 4, FB.Y −= 4 | `0x10` (middle path) |
| OM changes | 1, 2, 3, 4, 7, 8 and every use of blocks G, R and T |
| HT set | 7, `0x0F`, `0x10`, and block Q |

Effective blend rate per path (0.01 wherever not listed): selector 1 middle paths 0.1; 2: path 1 0.01 (stored), path 2 0.04; 3: paths 1–2 0.005, path 3 0.02; 4: path 1 0.005, path 2 0.01 (stored), path 3 0.005; 5 and 6: 0.005; 7: path 1 0.005, path 2 0.03; `0x0A` path 2: 0.03; `0x0F` and `0x10`: 0.03 when their spin term is taken; `0x11` and `0x12` middle paths: 0.05; `0x13` and `0x14`: 0.03.

The cases read these fields: owner signed 32-bit `+0x64`, bytes `+0x68`, `+0x140`, `+0x141`, `+0x990`, `+0x991`, `+0xAA8`, `+0xAA9`, the low byte of 16-bit `+0x7E`, binary32 triplets `+0x148` and `+0x154`, signed 32-bit `+0x1314`; C `+0x4`, `+0xDC`, `+0xE0`; the locals listed in Section 1.2; and, through the helpers, the inputs listed in the helper contract. The only call targets in these case bodies and the shared blocks they reach are B, Cq, R0, P, the model lookup, the two wrappers, the two imported routines, and the image's own small triplet routines (construct `0x00EA90`, copy `0x011E30`, add `0x012280`, component accessor `0x028970`, scale `0x051510`).

## 9. Random draws and server presence

No case body or shared block calls `rand`, `srand` or any routine other than those listed in Section 8; the timing helpers and P make no draws (helper contract section 10). There are no random draws.

The server image has no counterpart of these routines on the evidence of byte scans of `Server.exe`: none of the dispatch sequence at RVA `0x013AE9`, the post-evaluator entry at RVA `0x01E660`, the opening of the selector `0x0B` case, or three case-body immediate-store sequences (RVAs `0x019A79`, `0x01BB2A`, `0x01D14D`) occurs in it, while each occurs in the client. This is consistent with the helper contract's finding that the refresh routine is a client-only caller.

## 10. Unresolved

- The roles of owner `+0x148`, `+0x154`, `+0x1314`, `+0x140`, `+0x141`, `+0x64`, `+0x7E` and the slot kind and subtype codes; the identity of model collection entry 840.
- The consumers of OM and MM after RVA `0x01F0BE`, of FA and FB after RVA `0x023971`, and of FLAG at RVA `0x023A1C`.
- Behaviour when a divisor is zero (B, half(B), Cq or half(R0) equal to zero), when helper sums wrap, and when values are non-finite, beyond the operation order stated; the ambient MXCSR state; imported routine accuracy.
- The held contract states that the mount argument is read inside the selector `0x15` case; no such read was found in that case (the only read of that argument in the refresh is before the evaluator). This discrepancy is reported, not resolved here.
