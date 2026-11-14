Status: FINAL / STABLE / HELD

# Alpha pose evaluator: selectors 0x31 through 0x49

This independent source-side contract closes the per-selector case bodies of the client pose refresh for selectors `0x31` to `0x49` inclusive. The held contract `alpha-weapon-attachment-poses.md` (called "the poses contract" below) leaves these bodies bounded (its Section 4). That contract and `alpha-combat-helper-tables.md` were used only to locate the evaluator, its storage and the timing helpers. Every fact below was read again from the original.

## 1. Evidence

- Supported image `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base `0x00400000`. `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, was used only for Section 13. Both hashes were verified for this contract.
- All addresses are RVAs. "Image `X`" names data at RVA `X`, read from the file bytes through the section table. Every jump-table entry and every memory constant below was read that way. Immediate constants were read from the instruction encodings.
- Case bodies and shared blocks: `decompiled/client/assembly.txt` (range `0x013AFD–0x01E660`). Dispatch, pre-evaluator set-up and post-evaluator blend: `decompiled/client/assembly-functions/004128f0.asm`. Helpers: `decompiled/client/assembly.txt` with ranges from `decompiled/client/function_ranges.csv`. Import names: the import address table in the image bytes. Call closure: `decompiled/client/calls.csv`.
- Decompiler function files were not used. The Infinite-Cubes reconstruction was not consulted.

## 2. Terms

- **Owner** is the creature the pose subobject belongs to. **C** is the pose subobject, as in the poses contract. "Owner `+x`" is a creature field and "C `+x`" a subobject field.
- **Pose clock**: signed 32-bit C `+0x4`. **Action clock**: signed 32-bit owner `+0x6C`. A case compares one or the other, as stated.
- **T1**, **T2**, **R0**: the timing helpers RVA `0x03CAA0`, `0x047310` and `0x03D1A0`, closed in the helper-tables contract (there called B, C and R0). The case bodies always evaluate them on the owner. T1 and T2 always get the argument −1, which selects the owner's current action byte. Several cases evaluate the same helper more than once. Those helpers write nothing, so repeated evaluations in one case return the same value. Below, each helper value is named once even where the original evaluates it repeatedly.
- **I(n)**: a signed 32-bit integer converted to binary32 by the packed conversion (exact up to magnitude 2^24, otherwise rounded under the ambient MXCSR mode).
- **Sin(x)** and **Cos(x)** for a binary32 x: x widened to binary64, the imported MSVCR110 `_libm_sse2_sin_precise` or `_libm_sse2_cos_precise` applied, the result narrowed to binary32. Some cases reach these through the wrappers RVA `0x024B50` (sine) and RVA `0x00E420` (cosine). Each wrapper widens its binary32 argument, calls the import, narrows, and returns the binary32 through the x87 stack. The caller stores that to binary32, which is exact. Case `0x33` calls the cosine import directly with the same widen/narrow steps. The import thunks are RVA `0x28D9C2` (sine) and `0x28D7A4` (cosine), resolved through import slots image `0x2FC45C` and `0x2FC444`.
- Arithmetic is binary32 scalar unless binary64 is named. There are no fused operations. Parentheses give the exact grouping. Additions and subtractions of 32-bit integers wrap and compare signed.
- A **triplet** is three consecutive binary32 values X, Y, Z (components 0, 1, 2).
- "Exit" means a jump to the post-evaluator code at RVA `0x01E660` (poses contract Section 5). Every case in this range ends there; none returns early from the refresh.

## 3. Storage written by the evaluator

The evaluator works on stack locals of the refresh. Offsets are from the stack pointer at evaluator depth, which is the same as at the post-evaluator code. The mapping to C fields was read from the post-evaluator blend (`0x01E9D4–0x01F0A0`).

| Local | Name used here | Meaning in the poses contract |
|---|---|---|
| `+0x134` triplet | main-hand angle target | blended into C `+0x30` |
| `+0x164` triplet | off-hand angle target | blended into C `+0x24` |
| `+0x14C` triplet | main-hand offset target | blended into C `+0x18` |
| `+0x170` triplet | off-hand offset target | blended into C `+0x0C` |
| `+0x140` triplet | body-angle target | blended into C `+0x54` |
| `+0x194` triplet | creature-angle target | approached into C `+0x60` (angle helper) |
| `+0x1A0` triplet | head target | approached into C `+0x84` (angle helper) |
| `+0x200` triplet | tail target | approached into C `+0x90` (angle helper) |
| `+0x1AC` triplet | creature-offset target | blended into C `+0xAC`, `+0xB0`, `+0xB4` |
| `+0x1B8` triplet | C`+0x78` target | blended into C `+0x78` (last linear triplet) |
| `+0x1C4` triplet | hand-turn target | blended into C `+0x9C` |
| `+0x1F4` triplet | body-offset target | blended into C `+0x6C` |
| `+0x60` scalar | lift target | blended into C `+0xB8` |
| `+0x10` scalar | blend rate | rate of the approach that forms g |
| `+0x17C` triplet | main-hand rest position | gets C `+0x18` added at the end |
| `+0x158` triplet | off-hand rest position | gets C `+0x0C` added at the end |
| `+0xB0` scalar | off-hand weapon scale | uniform scale of poses contract Section 9 |

Auxiliary locals written by some cases in this range. The poses contract does not name them, and their later consumers are not traced here:

- `+0x188` triplet (**auxiliary off-side point**). Set before the evaluator (RVAs `0x013743–0x0137A5`) from intermediate rest-position values and owner `+0xF4` and `+0xFC`. That formula is not closed here. After the evaluator, before any C field is added (RVAs `0x01E66D–0x01E6AA`), it becomes (off-hand rest position + this point) × 0.5, component-wise.
- `+0x1D0` and `+0x1DC` triplets (**auxiliary points P and Q**). Set at RVAs `0x013825–0x01388D`: P = (owner `+0x100`, `+0x104`, `+0x108`), and Q is the same with an exact sign toggle of X.
- `+0x6C` scalar (**auxiliary scalar**). Set to +0 at RVA `0x01392E`.

Before the evaluator, the targets hold the values that the poses contract Section 3 gives: zero, except for the lift predicate and the owner `+0x124` bit `0x400` adjustments. The blend rate is 0.01 (bits `0x3C23D70A`, image `0x2FCDA0`). Where a case below "adds" to a target, it adds to that pre-evaluator value. Where it "sets" a target, earlier values are discarded.

Every triplet write in these cases stores whole triplets through the copy helper RVA `0x011E30` or the triplet builder RVA `0x00EA90`, unless a single component is named. Single components are reached through the element helper RVA `0x028970`, which returns the address of component k.

## 4. Dispatch facts

- The dispatch at RVA `0x013AE9` loads C `+0x8`, subtracts one, and jumps through image `0x023D4C` with index (selector − 1) when that value is at most `0x48` unsigned. So entry k serves selector k + 1.
- When the case bodies start, register XMM3 holds +0 and XMM4 holds 3.0 on every path (from RVAs `0x013A44`/`0x013AE6` and `0x01391B`/`0x013A3C`; 3.0 is bits `0x40400000`, image `0x2FCD70`). EDI holds C and ESI the model-collection argument. The cases below that compare with "+0" or add "3.0" use these registers.
- Table entries for this range:

| Selector | Case start | Note |
|---|---|---|
| `0x31` | `0x016D25` | |
| `0x32` | `0x01D62D` | |
| `0x33` | `0x01414F` | |
| `0x34` | `0x01409A` | runs on into `0x01414F` (selector `0x33`'s body) |
| `0x35` | `0x0142FC` | |
| `0x36` | `0x0143AB` | |
| `0x37` | `0x013F91` | |
| `0x38` | `0x01E576` | |
| `0x39` | `0x013AFD` | |
| `0x3A` | `0x013C3D` | |
| `0x3B` | `0x013E2B` | |
| `0x3C` | `0x018CDF` | |
| `0x3D` | `0x01B08B` | |
| `0x3E` | `0x01B3F0` | |
| `0x3F` | `0x018E84` | |
| `0x40` | `0x01E660` | goes straight to the post-evaluator code; no case body |
| `0x41` | `0x01A2FD` | |
| `0x42` | `0x017F9E` | |
| `0x43` | `0x01825E` | |
| `0x44` | `0x01B689` | |
| `0x45` | `0x01916A` | |
| `0x46` | `0x0144AF` | |
| `0x47` | `0x0183AC` | |
| `0x48` | `0x013BB1` | |
| `0x49` | `0x014A6E` | the same entry as selector `0x2C`; the shared body does not test the selector |

## 5. Shared sub-blocks

**Orbit offsets O.** Used by selector `0x3E` and the first branch of `0x3D` (code at `0x01B3F0–0x01B549` and `0x01B09D–0x01B1F6`). Steps, in order:
1. Body-angle target X = 20 and head target X = 20 (bits `0x41A00000`). Only these components are written.
2. a = I(pose clock) × 0.005 (bits `0x3BA3D70A`, image `0x2FCD9C`). Sin(a) is evaluated, then Cos(a). The off-hand offset target becomes ((Cos(a) × 3) − 1, 5, (Sin(a) × 5) + 5). Here 3 is image `0x2FCD70`, 1 is image `0x345DC0`, 5 is bits `0x40A00000` (image `0x2FCE24` for both uses in Z), and the Y value 5 is an immediate.
3. b = binary32 narrowing of (widened (I(pose clock) × 0.005) + binary64 pi). Pi is bits `0x400921FB54442D18`, image `0x2FCE08`; the product is formed again. Sin(b), then Cos(b). The main-hand offset target becomes ((Cos(b) × 3) + 1, 5, (Sin(b) × 5) + 5).

The binary32 scratch slot at local `+0x2C` holds a and then b. It is the same slot that held the pre-evaluator cycle value. No later read of that slot was traced.

**Tail W.** The block at `0x01E653–0x01E65B` stores a prepared triplet into the main-hand angle target. The block at `0x01E65A` stores a prepared triplet into whichever target the case selected. Neither block does anything else; both then fall into the post-evaluator code.

**Constant set K.** Used by selectors `0x39` and `0x34`:
- creature-offset target Z = −0.14 (bits `0xBE0F5C29`);
- body-angle target X = 10 (`0x41200000`);
- off-hand offset target = (0, 4, 0);
- main-hand offset target = (0, 4, 0);
- C`+0x78` target = (0, 2, −2).

## 6. Selector cases

Unless stated, every comparison of a clock with a helper result is signed 32-bit, and "x < y" means the case proceeds only when strictly less. No case in this range writes the pose clock.

### `0x31`

1. The hand-turn target is set to owner `+0x148`, `+0x14C`, `+0x150`.
2. **Pose clock < T1:**
   - creature-angle target Z = 30;
   - body-angle target = (0, 0, 90), stored one component at a time;
   - main-hand angle target = (−90, −90, 90);
   - the main-hand rest position becomes a copy of the off-hand rest position;
   - then off-hand rest X += 2 (image `0x345E10`), auxiliary off-side point Y += 10 (image `0x345E74`), and its Z −= 4 (image `0x345E60`);
   - blend rate = 0.005 (image `0x2FCD9C`);
   - the off-hand angle target becomes a copy of the main-hand angle target, (−90, −90, 90). Exit.
3. **Else, if pose clock < R0:**
   - body-angle target = (0, 0, 0), stored one component at a time;
   - T1 is evaluated once more and its result is discarded;
   - main-hand angle target = (−90, −90, 0);
   - if pose clock < T1 + T2, creature-offset target X = 0.25 (bits `0x3E800000`);
   - the off-hand rest position becomes a copy of the main-hand rest position;
   - then off-hand rest X −= 2, auxiliary off-side point Y += 10 and Z −= 4;
   - r = I(pose clock − T1) ÷ I(T2). If r > 1.0 (ordered), r = 1.0. There is no lower clamp, and a NaN r passes. A zero T2 gives an infinite or NaN quotient: +infinity clamps to 1.0, while −infinity and NaN pass unchanged;
   - creature-angle target Z = r × −360 (bits `0xC3B40000`, image `0x2FCEBC`);
   - blend rate = 0.03 (bits `0x3CF5C28F`, image `0x2FCDAC`);
   - the off-hand angle target becomes a copy of the main-hand angle target. Exit.
4. **Else (pose clock ≥ R0):**
   - the selector C `+0x8` is set to zero first;
   - the off-hand rest position becomes a copy of the main-hand rest position;
   - then off-hand rest X −= 2 and off-hand rest Z −= 1.0 (image `0x345DC0`), auxiliary off-side point Y += 10 and Z −= 4;
   - main-hand angle target = (−90, 90, 0);
   - body-angle target Z = 0;
   - the shared block at `0x016885` copies the main-hand angle target into the off-hand angle target twice (the same value). Blend rate unchanged. Exit.

Because this case sets the selector to zero, the post-evaluator code at `0x01E660` also zeroes the pose clock (poses contract Section 5).

### `0x32`

1. Unconditional first:
   - main-hand angle target = (60, −20, 20);
   - main-hand offset target = (0, 2, 4);
   - off-hand angle target = (60, 20, −20);
   - off-hand offset target = (0, 2, 4).
2. **Pose clock < T1:**
   - body-angle target Z = 50;
   - off-hand angle target X = −90 and Z = 90, giving (−90, 20, 90);
   - main-hand angle target X = −90, giving (−90, −20, 20);
   - off-hand offset target = (−4, −4, −2);
   - main-hand offset target = (0, 4, 1). Exit.
3. **Else, if pose clock < T1 + T2:**
   - p = (I(pose clock − T1) ÷ I(T2)) × 1.5 (image `0x2FCDE0`). If p > 1.0 (ordered), p = 1.0; otherwise, if p < +0 (ordered), p = +0. NaN passes;
   - creature-angle target Z = p × −270 (bits `0xC3870000`, image `0x2FCEB8`), then Y = p × −40 (bits `0xC2200000`, image `0x2FCEA0`);
   - body-offset target Z += 4 (image `0x345E60`);
   - p × 40 is formed and stored to the scratch slot but never read;
   - off-hand angle target X = −90 and Z = 90; main-hand angle target X = −90;
   - off-hand offset target = (−4, −4, −2); main-hand offset target = (0, 4, 1);
   - head target X = −30 and Z = −60 (Y unchanged);
   - blend rate = 0.03 (image `0x2FCDAC`). Exit.
4. **Else:**
   - body-angle target Z = 0; creature-angle target Z = 0;
   - main-hand angle target = (60, −20, 20) again;
   - a = I(pose clock) × 0.01 (image `0x2FCDA0`). Cos(a), then Sin(a). Main-hand offset target = (0, Sin(a) + 2, Cos(a) + 4), with 2 and 4 from images `0x345E10` and `0x345E60`;
   - off-hand angle target = (60, 20, −20);
   - b = narrowing of (widened (I(pose clock) × 0.01) + binary64 pi/2). Pi/2 is bits `0x3FF921FB54442D18`, image `0x2FCDF0`. Cos(b), then Sin(b). Off-hand offset target = (0, Sin(b) + 2, Cos(b) + 4);
   - c = Sin(I(pose clock) × 0.005). Body-offset target Z becomes (c × 0.5) + Z (0.5 from image `0x345DA0`, in the shared block `0x01A5A9`). Exit.

The scratch slot `+0x2C` again holds a and b.

### `0x33`

1. r = result of RVA `0x0C6B80` on the record at owner `+0x1E8` (Section 8). If the action clock ≥ r, exit with nothing written.
2. h = (Cos(I(pose clock) × 0.02) × 5) + 45. The cosine is the direct import call; 0.02 is bits `0x3CA3D70A` (image `0x2FCDA8`), 5 is image `0x2FCE24`, 45 is bits `0x42340000` (image `0x2FCE44`).
3. Then:
   - head target = (h, 0, 0);
   - body-angle target = (10, 0, 0);
   - creature-angle target = (10, 0, 0);
   - off-hand offset target = (1, 7, 8);
   - main-hand offset target = (−1, 7, 8);
   - off-hand angle target = (100, 0, 0);
   - main-hand angle target = (100, 0, 0). Exit.

### `0x34`

Constant set K (Section 5) is written, then execution continues at selector `0x33`'s body. So when `0x33`'s gate fails, K's writes remain and nothing else is written. When the gate passes, `0x33`'s writes follow; they rewrite the body-angle and both hand-offset targets and leave K's creature-offset Z and C`+0x78` target.

### `0x35`

- body-angle target = (−30, 0, 0);
- off-hand offset target = (4, 6, 6);
- main-hand offset target = (−4, 6, 6);
- body-offset target Y = 3. Exit.

### `0x36`

1. s = ((owner `+0x40`)² + (owner `+0x44`)²) + (owner `+0x48`)², with each square a binary32 product. If s is not greater than +0 (ordered, so an equal or unordered s fails), exit with nothing written.
2. Otherwise:
   - off-hand offset target Y += 4 (image `0x345E60`);
   - creature-angle target X = 20;
   - body-angle target X = −10;
   - head target X = −20;
   - off-hand angle target X = −20;
   - main-hand angle target X = −20;
   - main-hand offset target Y += 4;
   - auxiliary scalar = −40 (image `0x2FCEA0`);
   - auxiliary point Q Z += 3.0 and P Z += 3.0 (the dispatch register value);
   - P Y += 4 and Q Y += 4. Exit.

### `0x37`

- creature-angle target = (−90, 0, 0);
- head target X = 20;
- creature-offset target Z = ((owner `+0x84` − owner `+0x88`) × 0.5) − 0.15, with 0.5 from image `0x345DA0` and 0.15 = bits `0x3E19999A` (image `0x2FCDB8`);
- C`+0x78` target = (0, −1, 0);
- off-hand offset target = (0, 0, 3);
- main-hand offset target = (0, 0, 3). Exit.

### `0x38`

- body-angle target X += 30 (image `0x2FCD78`);
- off-hand offset target = (−2, 0, 0);
- main-hand offset target = (2, 0, 0);
- off-hand angle target = (−10, −10, 90);
- main-hand angle target = (−10, 10, −90). Exit.

### `0x39`

Constant set K (Section 5). Exit.

### `0x3A`

1. M = the mount argument (refresh argument 10). If M is nonnull, in this order:
   - creature-offset target Y −= 0.25 (image `0x345D98`);
   - creature-offset target Z becomes ((M `+0x88` × 0.75) + (V × 0.3)) + Z. 0.75 is image `0x2FCDC4` and 0.3 is image `0x2FCDC0`. V is the pre-evaluator local `+0x90`, defined below;
   - the offset triplet of RVA `0x046950` is formed for the signed 32-bit code at M `+0x64` (Section 8). Its components are added as X = r0 + X, Y = r1 + Y, Z = r2 + Z to the creature-offset target;
   - if M `+0x64` (32-bit) equals `0x16` or `0x17`, Z −= 0.75;
   - the lift target becomes the final Z with an exact sign toggle.
2. Then, unconditionally:
   - body-angle target X = 20;
   - off-hand offset target = (4, 6, −3);
   - main-hand offset target = (−4, 6, −3);
   - C`+0x78` target = (0, 2, −2);
   - off-hand angle target = (60, 90, 60);
   - main-hand angle target = (60, −90, −60). Exit.

V is set before the evaluator (RVAs `0x0133E6–0x01343C`). With W = owner `+0x1188` and the cycle value of poses contract Section 3 (narrowed sine stored at local `+0x2C`), V = (W × 4) × cycle, where 4 is image `0x345E60`. V is replaced by +0 when bit 1 of the low byte of owner 16-bit `+0x7E` is set and owner byte `+0x5C` bit 0 is clear.

### `0x3B`

1. Creature-offset target Z += 1.25 (image `0x2FCDD0`). The previous Z is read first.
2. Then:
   - body-angle target X = 20;
   - creature-angle target Z = −50;
   - off-hand offset target = (4, 6, −3);
   - main-hand offset target = (−4, 6, −3);
   - C`+0x78` target = (0, 1, 0);
   - head target = (0, 0, 30);
   - off-hand angle target = (60, 90, 60);
   - main-hand angle target = (60, −90, −60). Exit.

### `0x3C`

The pose clock is compared with the immediates 600 and 5000.

1. **Clock < 600:**
   - body-angle target Z = −80; creature-angle target Z = 0;
   - main-hand angle target = (−90, 90, −90);
   - off-hand angle target = (20, 90, 90). Exit.
2. **600 ≤ clock < 5000:**
   - body-angle target Z = 0;
   - creature-angle target Z = (I(clock − 600) × 360) ÷ 300, with 360 from image `0x2FCE6C` and 300 from image `0x2FCE68`;
   - off-hand angle target = (0, −90, 0);
   - main-hand angle target = (0, 90, 0) (shared block `0x018C2D`);
   - blend rate = 0.2 (bits `0x3E4CCCCD`, image `0x2FCDBC`). Exit.
3. **Clock ≥ 5000:**
   - body-angle target Z = 80; creature-angle target Z = 0;
   - off-hand angle target = (90, 90, 270);
   - main-hand angle target = (0, 90, 90). Exit.

### `0x3D`

1. **Pose clock < T1:** orbit offsets O (Section 5). Exit.
2. **Else, if pose clock < T1 + T2:**
   - body-angle target X = −20 and Z = 10;
   - off-hand offset target = (2, 4, 0);
   - main-hand offset target = (−2, 6, 0);
   - off-hand angle target = (0, 45, 0);
   - main-hand angle target = (0, −45, 0). Exit.
3. **Else, if pose clock < R0:**
   - off-hand offset target = (−3, −6, 0);
   - main-hand offset target = (3, −6, 0);
   - off-hand angle target = (−90, 0, 130);
   - main-hand angle target = (−90, 0, −130);
   - blend rate = 0.03 (image `0x2FCDAC`). Exit.
4. **Else:** exit with nothing written.

### `0x3E`

Orbit offsets O (Section 5), with no gate. Exit.

### `0x3F`

The values are E = pose clock − T1 + 100 (wrapping), q1 = E ÷ 500 truncated toward zero, and q2 = E ÷ 1000 truncated toward zero. Remainders are signed: −1, 0 or +1.

1. If pose clock ≥ (T2 + 400) + T1, exit with nothing written.
2. **Early branch**, taken when pose clock ≤ T1, or when the signed remainder of q1 by 2 equals +1 exactly (a remainder of −1 does not take it):
   - off-hand offset target = (2, −2, 12);
   - main-hand offset target = (−2, −2, 12);
   - body-angle target X = 20;
   - creature-angle target X = 10. Blend rate unchanged. Exit.
3. **Odd branch**, when the remainder of q2 by 2 is nonzero:
   - off-hand offset target = (2, −2, 12);
   - main-hand offset target = (0, 0, −5);
   - blend rate = 0.03 (image `0x2FCDAC`);
   - body-angle target = (−10, 0, 20);
   - creature-angle target = (−5, 0, 10). Exit.
4. **Even branch**, otherwise:
   - off-hand offset target = (0, 0, −5);
   - main-hand offset target = (−2, −2, 12);
   - blend rate = 0.03;
   - body-angle target = (−10, 0, −20);
   - creature-angle target = (−5, 0, −10). Exit.

The divisions use the multiply-high constant `0x10624DD3` with arithmetic shift 5 (for 500) or 6 (for 1000), plus the sign-bit correction. That is exact truncation toward zero for every signed 32-bit E.

### `0x40`

No case body (Section 4).

### `0x41`

1. If the action clock ≥ R0, exit with nothing written.
2. Otherwise:
   - body-angle target = (0, 0, Sin(I(pose clock) × 0.03) × 90). 0.03 is image `0x2FCDAC` and 90 is image `0x2FCE54`; the sine is reached through RVA `0x024B50`;
   - main-hand offset target = (0, 0, 0);
   - off-hand offset target = (0, 0, 0);
   - blend rate = 0.04 (bits `0x3D23D70A`, image `0x2FCDB0`). Exit.

### `0x42`

1. If owner binary32 `+0x13B4` > +0 (ordered), path A; otherwise (less, equal or unordered) path B.
2. **Path A:**
   - **Pose clock < T1:** body-angle target Z = −90; main-hand offset target = (2, −2, 1); main-hand angle target = (0, 10, 0); C `+0xE0` = C `+0xDC`.
   - **Else, if pose clock < T1 + 300:** body-angle target Z = 10; off-hand offset target = (−2, −2, 1); off-hand angle target = (0, −10, 0); C `+0xE0` = C `+0xDC`, then C `+0xDC` = null.
   - **Else, if pose clock < R0:** C `+0xDC` = null; body-angle target Z = 90; C `+0xE0` = null.
   - **Else:** nothing.
   - **Path A tail (all four cases):** blend rate = 0.03 (image `0x2FCDAC`). If pose clock ≤ T1 or pose clock ≥ R0, exit. Otherwise the creature-angle target Z = (I(pose clock − T1) × 360) ÷ 200 (images `0x2FCE6C`, `0x2FCE60`). If that value is greater than 700 (ordered; image `0x2FCE78`), it is replaced by 700. Exit.
3. **Path B:**
   - **Pose clock < T1:** body-angle target Z = −90; main-hand offset target = (2, −2, 1); main-hand angle target = (0, 10, 0). Exit.
   - **Else, if pose clock < R0:** body-angle target Z = 90; C `+0xDC` = null. Exit.
   - **Else:** exit with nothing written.

### `0x43`

1. Body-angle target Z = owner `+0x144` × −90 (image `0x2FCEB0`).
2. Main-hand offset target = (2, −2, 1); main-hand angle target = (0, 10, 0).
3. c = Cos(I(pose clock) × 0.5), through RVA `0x00E420`, with 0.5 from image `0x345DA0`. Main-hand rest position Z becomes (c × 0.5) + Z. Then c is formed again in the same way, and off-hand rest position Z becomes (c × 0.5) + Z.
4. If owner `+0x144` > +0 (ordered), C `+0xE0` = C `+0xDC`. Exit.

### `0x44`

1. **Pose clock < T1:**
   - body-angle target X = −20;
   - off-hand angle target = (−90, 0, 0);
   - main-hand angle target = (−90, 0, 0);
   - main-hand offset target = (1, −2, 0);
   - off-hand offset target = (−1, −2, 0). Exit.
2. **Else:**
   - body-angle target X = 0;
   - off-hand angle target = (0, 45, 0);
   - main-hand angle target = (0, −45, 0);
   - main-hand offset target = (−2, 5, 0);
   - off-hand offset target = (2, 6, 0). Exit.

### `0x45`

1. **Pose clock < T1 + T2:**
   - blend rate = 0.03 (image `0x2FCDAC`);
   - creature-angle target Z = (I(pose clock) × −360) ÷ I(T1 + T2), with −360 from image `0x2FCEBC`;
   - main-hand angle target = (−90, −90, −90);
   - main-hand offset target = (4, 3, 0);
   - body-angle target Z = (I(pose clock) × −60) ÷ I(T1 + T2), with −60 = bits `0xC2700000` (image `0x2FCEA8`);
   - off-hand angle target = (0, 0, 90). Exit.
2. **Else:**
   - off-hand angle target = (0, 0, 90);
   - main-hand angle target = (−60, −60, −60). Exit.

A zero T1 + T2 makes the quotients infinite or NaN; they are stored unchanged.

### `0x46`

1. If pose clock < T1: creature-angle target X = −30. Exit.
2. Else, if pose clock < T1 + T2: creature-angle target X = 120. Exit.
3. Otherwise nothing is written.

### `0x47`

1. **Action clock < T1:**
   - main-hand offset target = (−3, −2, 10);
   - off-hand offset target = (3, −2, 10);
   - C `+0xE0` = the model lookup RVA `0x0120C0` on the model-collection argument with index `0x835` (2101; Section 8);
   - off-hand weapon scale = scale × 1.5 (image `0x2FCDE0`);
   - body-angle target X = 40;
   - creature-angle target X = 10. Exit.
2. **Else, if action clock < T2 + T1:**
   - blend rate = 0.01 (image `0x2FCDA0`; equal to the default);
   - main-hand offset target = (0, 5, 0);
   - off-hand offset target = (0, 5, 0);
   - body-angle target X = −20;
   - creature-angle target X = −40. Exit.
3. **Else:**
   - main-hand offset target = (2, −1, 0);
   - off-hand offset target = (−2, −1, 0). Exit.

### `0x48`

1. If pose clock ≥ 5000, exit with nothing written.
2. Otherwise:
   - body-angle target X = 20;
   - head target X = 30;
   - off-hand offset target = (0, 0, 16);
   - main-hand offset target = (0, 0, 16). Exit.

### `0x49` (body shared with `0x2C`)

Let h1 = T2 ÷ 3 and h2 = (2 × T2, wrapping) ÷ 3, both truncated toward zero. They use the multiply-high constant `0x55555556` with sign correction, which is exact.

In every branch, "angle X" writes only component X of the off-hand and main-hand angle targets, and "offset Z" writes only component Z of the hand offset targets.

| Order | Condition (pose clock) | Head | Body angles | Body offset | C`+0x78` | Off-hand angle X | Main-hand angle X | Other hand writes | Tail |
|---|---|---|---|---|---|---|---|---|---|
| 1 | < T1 | (10, 0, 0) | (−20, 0, 0) | (0, −1, 0) | (0, 1, 1) | 20 | 20 | both offset Z = 2 | (30, 0, 10) |
| 2 | < T1 + h1 | (−30, 0, 0) | (50, 0, 0) | (0, 0, 1) | (0, −4, 2) | 60 | 70 | none | (−30, 0, 0) |
| 3 | < T1 + h2 | (−30, 0, 0) | (40, 0, 0) | (0, 1, 1) | (0, −2, 2) | 70 | −30 | main-hand offset = (0, 3, −2) | (−30, 0, −10) |
| 4 | < T1 + T2 | (−30, 0, 0) | (10, 0, 0) | (0, 1, 1) | (0, −2, 2) | −30 | −40 | off-hand offset = (0, 3, −2) | (−30, 0, −10) |
| 5 | < R0 | (10, 0, −10) | (−20, 0, 0) | (0, 1, 0) | (0, 1, 1) | 20 | 20 | both offset Z = 2 | (30, 0, 10) |
| 6 | otherwise | nothing is written | | | | | | | |

The first matching row decides. Each branch then exits.

## 7. Blend rate and selector writes

The effective blend rate is 0.01 unless one of these branches stores another value:

| Rate | Selector and branch |
|---|---|
| 0.005 | `0x31` first branch |
| 0.03 | `0x31` second branch; `0x32` third; `0x3D` third; `0x3F` odd and even; `0x42` every path-A outcome; `0x45` first |
| 0.04 | `0x41` when its gate passes |
| 0.2 | `0x3C` middle |
| 0.01 (explicit) | `0x47` middle |

All rate stores go to the stack slot `+0x10` at evaluator depth. That slot was confirmed for each site by tracking pushes in the case code. The only selector write in this range is in the third branch of `0x31`, which sets C `+0x8` to zero.

## 8. Helpers reached

- RVA `0x0C6B80` (closed). It reads a two-byte record (owner `+0x1E8`, `+0x1E9`). It returns 0 when the first byte is not 1. Otherwise it returns 3000 when the second byte is 1, and 10000 otherwise. Signed 32-bit result, no writes, no calls.
- RVA `0x046950` (closed). It takes an output-triplet address and a signed 32-bit code, and returns the output address. X and Y are always +0. Z is −0.9 (bits `0xBF666666`) for codes `0x1A` and `0x1E`; −0.2 (`0xBE4CCCCD`) for `0x21`; −0.3 (`0xBE99999A`) for `0x4A`; 0.1 (`0x3DCCCCCD`) for `0x4B`; −0.35 (`0xBEB33333`) for `0x97`; +0 for every other code, including codes below `0x1A` and above `0x97`. The dispatch subtracts `0x1A` and compares unsigned with `0x7D`. The index table is image `0x046A20` (126 bytes) and the target table image `0x046A08` (6 entries); both were decoded from the bytes. No reads beyond the arguments, no calls.
- RVA `0x0120C0` (closed). For index i: if i < 0, null. Otherwise, with the collection's begin and end words at `+0x4` and `+0x8`, it returns null when i ≥ ((end − begin) arithmetically shifted right by 2), and the 32-bit word at begin + 4·i otherwise.
- RVAs `0x011E30` (triplet copy), `0x00EA90` (triplet from three binary32 arguments), `0x028970` (component address): data moves only.
- T1, T2, R0: closed in the helper-tables contract. They are pure, binary32, with no draws (re-confirmed below).

## 9. Writes outside the targets

Besides the target locals and the blend rate, cases in this range write:

- **Rest positions:** `0x31`, and `0x43` (Z components).
- **Auxiliary off-side point:** `0x31`.
- **Auxiliary points P and Q, and the auxiliary scalar:** `0x36`.
- **Lift target:** `0x3A`.
- **Off-hand weapon scale local:** `0x47`.
- **C `+0xDC`:** `0x42`.
- **C `+0xE0`:** `0x42`, `0x43`, `0x47`. These writes follow the pre-evaluator clear of C `+0xE0` for the item kind of poses contract Section 3, so they can restore an off-hand model there.
- **Selector C `+0x8`:** `0x31`.

No case in this range writes the pose clock, any rest hand-angle local, any zero-turn local, any matrix, or any series point.

## 10. Owner and argument fields read

| Field | Width | Read by |
|---|---|---|
| owner `+0x40`, `+0x44`, `+0x48` | binary32 | `0x36` |
| owner `+0x6C` (action clock) | signed 32-bit | `0x33`, `0x41`, `0x47` |
| owner `+0x84`, `+0x88` | binary32 | `0x37` |
| owner `+0x144` | binary32 | `0x43` |
| owner `+0x148`–`+0x150` | binary32 triplet | `0x31` |
| owner `+0x13B4` | binary32 | `0x42` |
| owner `+0x1E8`, `+0x1E9` | bytes | `0x33` and `0x34`, through RVA `0x0C6B80` |
| mount `+0x88` | binary32 | `0x3A` |
| mount `+0x64` | 32-bit | `0x3A` |
| C `+0x4` (pose clock) | signed 32-bit | every timed case except the action-clock gates |
| C `+0xDC` | pointer | `0x42`, `0x43` |
| model collection | pointer | `0x47` |
| pre-evaluator local V | binary32 | `0x3A` |

The timing helpers additionally read the owner fields stated in the helper-tables contract.

## 11. Random draws

None. The case bodies in this range call only RVAs `0x011E30`, `0x00EA90`, `0x028970`, `0x03CAA0`, `0x047310`, `0x03D1A0`, `0x0C6B80`, `0x046950`, `0x0120C0`, `0x024B50`, `0x00E420` and the cosine import thunk. Their transitive direct-call closure in `decompiled/client/calls.csv` has 19 routines. It contains no computed call and reaches only the imported sine, cosine and pow. Neither `rand` nor `srand` is reached.

## 12. Notes against the held poses contract

These original facts differ from the bounds in the poses contract Section 4:

- That section says the mount argument is read only in selector `0x15`'s case. Selector `0x3A` also reads it.
- Its list of case-body writes does not mention the weapon model pointers or the off-hand scale local. Selectors `0x42`, `0x43` and `0x47` write C `+0xDC`/`+0xE0`, and selector `0x47` writes the off-hand scale local.
- Cases in this range also write auxiliary locals that the poses contract does not describe (Section 3 above).

Outside the scope of this contract, but observed in passing: the owner `+0x124` bit `0x400` branch before the evaluator also sets the head target X to 70 and the C`+0x78` target to (0, −1, 0).

## 13. Server

`Server.exe` has no counterpart of this evaluator. A byte scan of the whole server image finds:

- no instance of the dispatch idiom (compare with `0x48` followed by a long unsigned-above jump), which occurs exactly once in the client, at the evaluator dispatch;
- no instance of the case immediate bits `0xBE0F5C29` or `0x442F0000`, both present in the client case bodies.

The server listing has no reference to these case constants inside a pose routine. This agrees with the helper-tables contract, which lists RVA `0x0128F0` among the client-only callers.

## 14. Unresolved

- The later consumers of the auxiliary locals (auxiliary off-side point, P, Q, auxiliary scalar) and of the scratch slot `+0x2C` after the evaluator were not traced.
- What the owner fields read here represent (for example `+0x144`, `+0x148`, `+0x13B4`, and the `+0x1E8` record), what the mount's `+0x64` code means, and which models collection entry 2101 holds.
- Gameplay meaning of each selector and of the timing values. The internal formula of the attack-speed query that T1, T2 and R0 may call is closed elsewhere, not here.
- Ambient MXCSR and x87 state, denormals, imported math accuracy, non-finite inputs beyond the cases stated, wrapping overflow in the clock sums, and invalid pointers are limits outside this contract.
