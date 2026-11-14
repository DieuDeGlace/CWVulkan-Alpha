Status: FINAL / STABLE / HELD

# Alpha pose evaluator: selectors 0x19 through 0x24

This independent source-side contract closes the case bodies of the per-selector pose evaluator for selectors `0x19` through `0x24` inclusive. The held contract `alpha-weapon-attachment-poses.md` (Section 4) leaves this evaluator bounded; it was used only to locate the evaluator, its storage names and the post-evaluator code. The held contract `alpha-combat-helper-tables.md` was used only to name the timing helpers. Every fact below was read again from the original.

## Evidence

- Supported original `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred base `0x00400000` (hash verified for this contract). All addresses are RVAs; "image `X`" names data at image RVA `X`.
- `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` (hash verified), used only for the presence check of Section 9.
- Instruction listing `decompiled/client/assembly.txt`, read for the dispatch (RVAs `0x013AE9–0x013AFC`), the case bodies listed in Section 2, the pre-evaluator target initialisation (`0x013919–0x013AE9`), the rest hand-position stores (`0x0134E3–0x013743`), the post-evaluator blend and final additions (`0x01E660–0x01F0A0`) and the small helpers named in Section 1.4.
- Image bytes: the 73-entry dispatch table at image `0x023D4C`, every binary32 constant read through memory, and the import address table entries named in Section 1.4.
- `decompiled/client/calls.csv`, for the random-draw closure (Section 8).
- The Infinite-Cubes reconstruction was not consulted.

## 1. Terms

### 1.1 Objects and helpers

- **C**: the pose subobject (held contract Section 1), kept in the `EDI` register throughout the evaluator. **Owner**: the creature pointed to by C `+0x0`. Owner offsets are written "owner `+x`".
- **Clock**: the signed 32-bit pose clock C `+0x4`. **Selector**: the signed 32-bit C `+0x8`.
- **T1**: RVA `0x03CAA0` (helper-tables "B"); **T2**: RVA `0x047310` ("C"); **A**: RVA `0x044270` ("A"); **R0**: RVA `0x03D1A0`. Every call in these case bodies passes the owner as receiver; T1, T2 and A always receive the stack argument −1, so each uses the owner's action byte `+0x68`. R0 takes no argument. Each call is a fresh evaluation; repeated calls in one case body return the same value because the helpers are pure (helper-tables contract, Sections 2–5). All results are signed 32-bit.
- Integer sums and differences of these results and of the clock are 32-bit wrapping operations. All comparisons of the clock with them are signed 32-bit. "Halved toward zero" means the signed division by 2 that truncates toward zero.

### 1.2 Stack locals and their held-contract names

All case bodies run with the same frame as the code before and after the evaluator, so each local is named by its offset from the stack pointer at the dispatch ("stack `+x`"). Every store address below was computed with the pushes and stack adjustments in force at that instruction. The names come from where the post-evaluator code consumes each local:

| Stack local | Held-contract name (consumer) |
|---|---|
| `+0x10` | Blend-rate slot (binary32; pushed to the repeated approach at RVA `0x01E9D4`) |
| `+0x134` | Main-hand angle target (blended into C `+0x30`) |
| `+0x140` | Body-angle target (C `+0x54`) |
| `+0x14C` | Main-hand offset target (C `+0x18`) |
| `+0x158` | Off-hand rest hand position (receives C `+0x0C` at RVA `0x01F095`) |
| `+0x164` | Off-hand angle target (C `+0x24`) |
| `+0x170` | Off-hand offset target (C `+0x0C`) |
| `+0x17C` | Main-hand rest hand position (receives C `+0x18` at RVA `0x01F085`) |
| `+0x188` | Off-hand auxiliary triplet: not named by the held contract. At RVA `0x01E66D` it is replaced by (off-hand rest position + itself) × 0.5; its later use is outside this contract |
| `+0x194` | Creature-angle target (C `+0x60`, angle approach) |
| `+0x1A0` | Head-angle target (C `+0x84`, angle approach) |
| `+0x1AC` | Creature-offset target (C `+0xAC`) |
| `+0x1C4` | Hand-turn target (C `+0x9C`) |
| `+0x27` (byte) | Flag byte, cleared at RVA `0x013941`; read only at RVA `0x023A1C`, where a set flag selects a Z rotation by 60 for a matrix built after the held contract's sections. That consumer is outside this contract |

Component k of a triplet at stack `+x` is the binary32 word at `+x + 4k` (k = 0, 1, 2 for X, Y, Z). The case bodies in this contract write no other named local. In particular they do not write the lift target (stack `+0x60`), the turn-scalar target (`+0x74`), the C `+0x78` target (`+0x1B8`), the body-offset target (`+0x1F4`), the tail-angle target (`+0x200`), the rest hand-angle locals (`+0x20C`, `+0x224`), the main-hand auxiliary triplet (`+0x23C`), or any C field other than the selector.

They also use stack words `+0x0C`, `+0x14`, `+0x18`, `+0x28`, `+0x2C` and `+0x30` as scratch. None of these is one of the named locals above or the blend amount (stack `+0x1C`). Selectors `0x1E` and `0x1F` leave a final value in `+0x30` (Sections 5.6 and 5.7). Whether code after the evaluator reads any of these words before rewriting them was not traced.

### 1.3 Effect notation

- "T := (x, y, z)" replaces all three components of target T with binary32 values whose bit patterns are listed in Section 1.5. "T.Y := v" replaces one component.
- "T.Y += v" and "T.Y −= v" are one binary32 add or subtract with one rounding, applied to whatever value the component holds when the instruction runs. On entry to a case body that is the pre-evaluator value (held contract Section 3; for example, the hand-offset targets are nonzero when owner `+0x124` bit `0x400` is set). The constant is the second operand.
- "U := V" for two triplets copies the three words bit-exactly.
- Unless stated, a branch does not write the blend-rate slot, which then keeps binary32 0.01 (bits `0x3C23D70A`, image `0x2FCDA0`, stored at RVA `0x013946`). Every rate store in this contract targets stack `+0x10`.
- No case body in this range writes the selector, except selectors `0x1D` and `0x23` (Sections 5.5 and 5.11). A zero selector makes the post-evaluator code zero the clock at RVA `0x01E666` (held contract Section 5).
- Every branch ends with a transfer to the post-evaluator code at RVA `0x01E660`. Some branches first pass through the shared copy tail at RVA `0x01E65A`, which copies one triplet into another before reaching `0x01E660`.

### 1.4 Numeric primitives

- **Integer to binary32**: packed conversion of the signed 32-bit value (exact up to magnitude 2^24, otherwise rounded under the ambient MXCSR mode).
- **Sine and cosine**: wrapper RVA `0x024B50` (sine) and RVA `0x00E420` (cosine). Each widens its binary32 argument exactly to binary64, calls the imported MSVCR110 precise SSE2 function (image IAT words `0x2FC45C` and `0x2FC444` name `_libm_sse2_sin_precise` and `_libm_sse2_cos_precise`), narrows the result to binary32 under the MXCSR mode and returns it on the x87 stack. The caller stores it as binary32 (exact, since the value is already binary32).
- Small helpers, all re-read: `0x00EA90` stores three stack values into a triplet; `0x011E30` copies a triplet; `0x028970` returns the address of component k; `0x012280` forms (other + this) component-wise, one binary32 add each; `0x051510` forms vector × scalar component-wise, one binary32 product each. None reads or writes anything else.
- **Blend of two vectors** (selectors `0x1E`, `0x1F`, `0x20`): "(V × p) + (W × q)" means, per component, the binary32 product V_k × p and the binary32 product W_k × q, each rounded, then their binary32 sum.
- **Clamp to one**: after a binary32 quotient p is formed, it is compared with 1.0 (bits `0x3F800000`, image `0x345DC0`); when p is ordered greater than 1.0 it becomes exactly 1.0. A NaN quotient is not clamped, and negative values are not raised. Then q = 1.0 − p (one binary32 subtraction).

### 1.5 Constants

Values loaded from memory (all binary32, read from the image):

| Value | Bits | Image |
|---|---|---|
| 0.005 | `0x3BA3D70A` | `0x2FCD9C` |
| 0.02 | `0x3CA3D70A` | `0x2FCDA8` |
| 0.03 | `0x3CF5C28F` | `0x2FCDAC` |
| 0.04 | `0x3D23D70A` | `0x2FCDB0` |
| 0.05 | `0x3D4CCCCD` | `0x2FCDB4` |
| 0.2 | `0x3E4CCCCD` | `0x2FCDBC` |
| 0.8 | `0x3F4CCCCD` | `0x2FCDC8` |
| 1.2 | `0x3F99999A` | `0x2FCDCC` |
| 1.0 | `0x3F800000` | `0x345DC0` |
| 0.5 | `0x3F000000` | `0x345DA0` |
| +0.0 | `0x00000000` | `0x345D84` |
| −0.0 | `0x80000000` | `0x2FCE80` |
| 2 | `0x40000000` | `0x345E10` |
| 3 | `0x40400000` | `0x2FCD70` |
| 4 | `0x40800000` | `0x345E60` |
| 10 | `0x41200000` | `0x345E74` |
| 20 | `0x41A00000` | `0x2FCD74` |
| 30 | `0x41F00000` | `0x2FCD78` |
| −20 | `0xC1A00000` | `0x2FCE9C` |
| −30 | `0xC1F00000` | `0x345E9C` |
| 45 | `0x42340000` | `0x2FCE44` |
| 90 | `0x42B40000` | `0x2FCE54` |
| 360 | `0x43B40000` | `0x2FCE6C` |

Immediate stores are exact binary32 values with these bit patterns: 0 `0x00000000`, −0 `0x80000000`, ±0.5 `0x3F000000`/`0xBF000000`, ±1 `0x3F800000`/`0xBF800000`, 2 `0x40000000`, 3 `0x40400000`, 4 `0x40800000`, 6 `0x40C00000`, 8 `0x41000000`, 9 `0x41100000`, 14 `0x41600000`, −7 `0xC0E00000`, −8 `0xC1000000`, −10 `0xC1200000`, −12 `0xC1400000`, ±20 `0x41A00000`/`0xC1A00000`, ±30 `0x41F00000`/`0xC1F00000`, ±40 `0x42200000`/`0xC2200000`, ±45 `0x42340000`/`0xC2340000`, 48 `0x42400000`, 50 `0x42480000`, ±60 `0x42700000`/`0xC2700000`, ±80 `0x42A00000`/`0xC2A00000`, ±90 `0x42B40000`/`0xC2B40000`, 100 `0x42C80000`, ±110 `0x42DC0000`/`0xC2DC0000`, −120 `0xC2F00000`, −130 `0xC3020000`, −180 `0xC3340000`, 270 `0x43870000`.

## 2. Dispatch

At RVA `0x013AE9` the selector minus one is compared unsigned with `0x48`; larger values go to `0x01E660`, otherwise the table at image `0x023D4C` is indexed by selector − 1. Entries for this range, read from the image:

| Selector | Table entry (image) | Case body (RVA) |
|---|---|---|
| `0x19` | `0x023DAC` | `0x01AB1A` |
| `0x1A` | `0x023DB0` | `0x01A80B` |
| `0x1B` | `0x023DB4` | `0x01A3E6` |
| `0x1C` | `0x023DB8` | `0x018AC6` |
| `0x1D` | `0x023DBC` | `0x019347` |
| `0x1E` | `0x023DC0` | `0x01C115` |
| `0x1F` | `0x023DC4` | `0x01C67C` |
| `0x20` | `0x023DC8` | `0x01CBDA` |
| `0x21` | `0x023DCC` | `0x01CF8B` |
| `0x22` | `0x023DD0` | `0x019A23` |
| `0x23` | `0x023DD4` | `0x019748` |
| `0x24` | `0x023DD8` | `0x01E660` (straight to the post-evaluator code) |

Selector `0x24` is the only entry in this range that goes straight to the post-evaluator code; it writes nothing in the evaluator. No case body in this range reads a register value set before the dispatch other than C and the frame.

## 3. Shared blocks

### 3.1 Oscillation O (selectors `0x19`, `0x1A`, `0x1B`)

Here c = binary32 of the clock (the clock is re-read and converted for each line). In this order:

1. Main-hand offset target.Y += 4; off-hand offset target.Y += 4.
2. Main-hand offset target.X += sine(c × 0.04) × 0.5.
3. Main-hand offset target.Z += cosine(c × 0.05) × 0.5.
4. Off-hand offset target.X += sine(c × 0.04) × 0.5.
5. Off-hand offset target.Z += cosine(c × 0.05) × 0.5.

In each of lines 2–5 the argument c × K is one binary32 product. The narrowed sine or cosine is multiplied by 0.5 in binary32, and that product is the first operand of the binary32 addition to the component. Four imported calls are made, sine, cosine, sine, cosine.

### 3.2 Arms-forward block F (selectors `0x1A` first branch, `0x1B`)

Body-angle target.Z := −30; main-hand angle target := (0, −45, −60); off-hand angle target := (−45, −45, 0); then O. The selector `0x1B` body (RVAs `0x01A3E6–0x01A5C1`) and the first branch of selector `0x1A` (RVAs `0x01A81D–0x01A9E5`, which finishes in the `0x1B` code at RVA `0x01A5A9`) are separate copies with the same instructions and constants.

### 3.3 Mirror tail M

There are two copies: RVAs `0x01CB39–0x01CBDA` (used by selectors `0x1F`, `0x20`, `0x21`) and RVAs `0x01C5AD–0x01C67C` (selector `0x1E`). Each entry point at `0x01CB39` or `0x01C5AD` first stores the rate value supplied by the caller into the blend-rate slot. Entry at `0x01CB3F` or `0x01C5B3` skips that store. Then, in order:

1. Off-hand offset target := main-hand offset target.
2. Off-hand angle target := main-hand angle target.
3. Off-hand rest position := main-hand rest position.
4. Off-hand rest position.X −= 0.2; .Y −= 0.2; .Z −= 1.0.

The `0x1E` copy then evaluates (owner signed 32-bit `+0x1314` halved toward zero), takes its signed remainder modulo 3, and when that remainder equals 2 calls T1 and T2 and discards both results. This changes no state.

### 3.4 Rest block E (selectors `0x1F`, `0x20`, `0x21`)

RVAs `0x01CAF1–0x01CB39`: main-hand angle target := (0, 0, 0); body-angle target.Z := −30; rate value 0.03, then M from `0x01CB39`, so the effective rate is 0.03.

### 3.5 Two-handed block S (selectors `0x1D`, `0x23`)

RVAs `0x0195C3–0x019748`, in order:

1. Off-hand angle target := (−80, 40, 60).
2. Off-hand offset target.Y −= 4; .Z += 2.
3. If C `+0xDC` (main-hand model pointer) is null, exit. If C `+0xE0` (off-hand model pointer) is nonnull, exit. Both tests are full 32-bit comparisons with zero.
4. Main-hand rest position.Y += 2; .Z += 3.
5. Off-hand rest position := main-hand rest position (the updated values).
6. Off-hand rest position.X −= 3; .Z −= 3.
7. Off-hand auxiliary triplet (stack `+0x188`).Y += 10; .Z −= 4.
8. Main-hand angle target.X := 0; .Y := 20.
9. Off-hand angle target := main-hand angle target (through the copy tail at `0x01E65A`).

## 4. Phase thresholds

Most selectors in this range choose one of up to three branches by comparing the clock with thresholds:

| Name | Value |
|---|---|
| E1 | T1 |
| E2 | R0 |
| E2′ | T1 + T2 |
| E2″ | (T1 + T2) + A |

Each phase is "clock < threshold" in signed 32-bit comparison. A later phase runs only when every earlier comparison failed.

## 5. Case bodies

### 5.1 Selector `0x19` (RVAs `0x01AB1A–0x01AD90`)

1. If clock < T1: O; then body-angle target.Z := −20. Done.
2. Otherwise, if clock ≥ R0: done, with no write.
3. Otherwise: creature-offset target.X := −0.5; body-angle target.Z := 60; creature-angle target.Z := 30; main-hand angle target := (0, −45, −60); off-hand angle target := (−45, −45, 0); main-hand offset target := (2, 0, 0); flag byte := 1. Done.

The rate is 0.01 in every branch.

### 5.2 Selector `0x1A` (RVAs `0x01A80B–0x01AB1A`)

1. If clock < T1: F (Section 3.2). Rate 0.01.
2. Otherwise, if clock < R0: body-angle target.Z := 90; main-hand angle target := (−90, 0, −90); off-hand angle target := (−45, −45, 0); main-hand offset target := (3, 0, 1); rate := 0.03.
3. Otherwise: main-hand angle target := (−60, 0, 0); main-hand offset target := (3, 0, 1) (through the copy tail). Rate 0.01.

### 5.3 Selector `0x1B` (RVAs `0x01A3E6–0x01A5C1`)

There is no branch: F (Section 3.2). No helper is called. Rate 0.01.

### 5.4 Selector `0x1C` (RVAs `0x018AC6–0x018CDF`)

1. If clock < T1: body-angle target.Z := −80; creature-angle target.Z := +0; main-hand angle target := (−90, 90, −90); off-hand angle target := (20, 90, 90). Rate 0.01.
2. Otherwise T1 and T2 are evaluated, and their sum E2′ is formed. If clock < E2′:
   - body-angle target.Z := −90;
   - creature-angle target.Z := (binary32 of (clock − T1) × 360) ÷ binary32 of T2. T1 and T2 are evaluated again. The integer difference wraps, then there is one product and one quotient, each rounded; there is no clamp;
   - off-hand angle target := (0, 100, 90); main-hand angle target := (0, 90, 0); rate := 0.2.
3. Otherwise: body-angle target.Z := 80; creature-angle target.Z := +0; off-hand angle target := (90, 90, 270); main-hand angle target := (0, 90, 90) (through the copy tail). Rate 0.01.

The selector is not written.

### 5.5 Selector `0x1D` (RVAs `0x019347–0x0195C3`, then S)

1. If clock < T1: main-hand angle target := (−80, −90, 60); off-hand angle target := (−80, 90, −60); main-hand offset target.Y += 3; main-hand offset target.X −= 5; off-hand offset target.X += 5. Rate 0.01.
2. Otherwise, if clock < R0: body-angle target := (0, 0, −30); main-hand offset target.Y −= 5; off-hand offset target.Y −= 5; off-hand angle target := (−90, 90, 110); main-hand angle target := (−90, −90, −110); rate := 0.04; creature-offset target.Y := −0.5.
3. Otherwise:
   - hand-turn target := (0, 0, 0); selector := 0;
   - off-hand angle target.X := −90; body-angle target.Z := 20; main-hand angle target := (−120, −90, 30);
   - then S (Section 3.5). Rate 0.01.

   S step 1 overwrites the off-hand angle target, so the X store above has no lasting effect. When S exits at step 3, the off-hand angle target is (−80, 40, 60) and the main-hand angle target (−120, −90, 30). When S completes, both are (0, 20, 30).

### 5.6 Selector `0x1E` (RVAs `0x01C115–0x01C67C`)

The clock is read at entry, then T1 is evaluated. That clock read serves the first comparison and r; later comparisons read the clock again (same value).

**Branch 1: clock < T1.** T1 is evaluated again. r = binary32 of clock ÷ binary32 of T1 (one rounded quotient, no clamp). Then:

1. Main-hand angle target := (−80, 90, −90); main-hand offset target := (−10, 8, 8); body-angle target.Z := −45.
2. Creature-angle target.Z := r × −20 (constant `0x2FCE9C`).
3. Creature-offset target.X := r × +0.0 (constant `0x345D84`). This gives a signed zero for finite r and NaN for an infinite or NaN r.
4. h = owner signed 32-bit `+0x1314` halved toward zero. If the signed remainder of h modulo 3 equals 2: T2 is evaluated, then T1. If clock < T1 + (T2 halved toward zero), then creature-angle target.Z += r × 360.
5. Rate value 0.04, then M from `0x01C5AD`, so the effective rate is 0.04.

**Branch 2: clock ≥ T1 and clock < E2″.** T1, T2 and A are evaluated in that order for E2″. Then T1 and T2 are evaluated again, and:

1. p = (binary32 of (clock − T1) × 1.2) ÷ binary32 of T2, with clamp to one. Then q = 1 − p.
2. Creature-offset target.X := (q × 0.0) − (p × 0.8).
3. Main-hand angle target := ((−110, 60, 0) × p) + ((−90, 90, 48) × q).
4. Main-hand offset target := ((−12, 0, −1) × p) + ((−10, 8, 8) × q).
5. Head-angle target.X := −10.
6. Body-angle target.Z := (p × 90) − (q × 45).
7. Creature-angle target.Z := p × 30.
8. Rate := 0.05. Stack `+0x30` := p × 10 (no reader identified).
9. M from `0x01C5B3`.

**Branch 3: otherwise.** Main-hand offset target := (2, 0, 0); main-hand angle target := (0, 0, 0); body-angle target.Z := −30; rate value 0.03; M from `0x01C5AD`, so the effective rate is 0.03.

### 5.7 Selector `0x1F` (RVAs `0x01C67C–0x01CBDA`)

**Branch 1: clock < T1.**

1. Main-hand angle target := (0, −90, 0); main-hand offset target := (−8, 6, 6); head-angle target.X := −10; head-angle target.Z := 30; body-angle target.Z := 45; rate := 0.05; creature-offset target.X := −0.0; creature-angle target.Z := 20.
2. T1 is evaluated again. r = binary32 of clock ÷ binary32 of T1.
3. If (owner `+0x1314` halved toward zero) has signed remainder 2 modulo 3: T2 and then T1 are evaluated. If clock < T1 + (T2 halved toward zero), then creature-angle target.Z := creature-angle target.Z − (r × 360).
4. M from `0x01CB3F`. The effective rate is 0.05.

**Branch 2: clock ≥ T1 and clock < R0.** After R0, T1 (again) and then T2 are evaluated, and:

1. p = (binary32 of (clock − T1) × 1.2) ÷ binary32 of T2, with clamp to one. Then q = 1 − p.
2. Creature-offset target.X := (p × 0.8) + (q × −0.0).
3. Head-angle target.Z := −30.
4. Main-hand angle target := ((0, −90, −130) × p) + ((0, −90, 20) × q).
5. Main-hand offset target := ((0, 2, 0) × p) + ((−8, 6, 6) × q).
6. Body-angle target.Z := (q × 45) − (p × 45).
7. Creature-angle target.Z := p × −30; creature-angle target.X := p × −20.
8. Rate := 0.05. Stack `+0x30` := p × 10 (no reader identified).
9. M from `0x01CB3F`.

**Branch 3: otherwise.** Main-hand offset target := (2, 0, 0); then E (Section 3.4). The effective rate is 0.03.

### 5.8 Selector `0x20` (RVAs `0x01CBDA–0x01CF8B`)

The clock is read at entry, then T1 is evaluated, as in Section 5.6.

**Branch 1: clock < T1.** T1 is evaluated again. r = binary32 of clock ÷ binary32 of T1. Then:

1. Main-hand offset target := (2, 2, 14); main-hand angle target := (50, 0, 0); body-angle target.Z := −45.
2. Creature-angle target.Z := r × −20; creature-offset target.X := r × 0.5.
3. Rate value 0.04, then M from `0x01CB39`, so the effective rate is 0.04.

**Branch 2: clock ≥ T1 and clock < E2″.** T1, T2 and A are evaluated in that order. Then T1 and T2 are evaluated again, and:

1. p = (binary32 of (clock − T1) × 2) ÷ binary32 of T2, with clamp to one. Then q = 1 − p.
2. Creature-angle target.X := (q × 20) − (p × 10).
3. Main-hand angle target := ((−90, 0, −45) × p) + ((90, 0, 45) × q).
4. Main-hand offset target := ((0, 9, 0) × p) + ((2, 2, 14) × q).
5. Body-angle target.Z := (p × 45) − (q × 45).
6. Rate value 0.05, then M from `0x01CB39`.

**Branch 3: otherwise.** Main-hand offset target := (2, 0, 0); then E. The effective rate is 0.03.

### 5.9 Selector `0x21` (RVAs `0x01CF8B–0x01D123`)

1. If clock < T1: main-hand offset target := (4, 2, 8); main-hand angle target := (−90, −180, 90); body-angle target.Z := −90; creature-offset target.Y := −1; rate value 0.02, then M from `0x01CB39`, so the effective rate is 0.02.
2. Otherwise T1 and T2 are evaluated. If clock < E2′: main-hand offset target := (6, 0, 0); creature-offset target.Y := 0.5; body-angle target.Z := 90; main-hand angle target := (−90, 0, −90); flag byte := 1; rate value 0.05, then M from `0x01CB39`.
3. Otherwise: main-hand offset target := (2, 0, 0); then E. The effective rate is 0.03.

### 5.10 Selector `0x22` (RVAs `0x019A23–0x019A6F`)

There is no branch and no helper call: main-hand angle target := (−90, −90, 80); body-angle target.Z := −80. Rate 0.01.

### 5.11 Selector `0x23` (RVAs `0x019748–0x019A23`, then S)

1. If clock < T1: main-hand angle target := (−90, 0, 0); off-hand angle target := (−90, 0, 40); main-hand offset target := (1, −7, 0); off-hand offset target := (−1, −7, 0); body-angle target.Z := −40; creature-offset target.Y := −0.5; rate := 0.005; flag byte := 1.
2. Otherwise, if clock < R0: main-hand angle target := (−90, −60, −30); off-hand angle target := (−90, 60, −50); main-hand offset target := (3, 5, 2); off-hand offset target := (6, 8, 2); body-angle target.Z := 40; rate := 0.05; flag byte := 1.
3. Otherwise:
   - hand-turn target := (0, 0, 0); selector := 0.
   - If owner signed 32-bit `+0x64` equals `0x60` or `0x57`, done (rate 0.01).
   - Otherwise: off-hand angle target.X := −60; body-angle target.Z := 20; main-hand angle target := (−120, −90, 30); then S. Rate 0.01. S overwrites the off-hand angle X store, as in Section 5.5.

### 5.12 Selector `0x24`

The table entry goes straight to RVA `0x01E660`; the evaluator writes nothing.

## 6. Summary of reads

- **C fields**: the clock C `+0x4` (signed 32-bit; read at every phase test and, as binary32, in O and in the r and p formulas) and, in S only, the model pointers C `+0xDC` and `+0xE0` (null tests). No case body in this range reads the selector.
- **Owner fields read directly**: signed 32-bit `+0x64` (selector `0x23`, third branch) and signed 32-bit `+0x1314` (selectors `0x1E` and `0x1F`). The timing helpers read their own inputs (helper-tables contract).
- **Helpers called**: T1, T2, A and R0, the two trigonometric wrappers, and the small triplet helpers of Section 1.4. The model lookup RVA `0x0120C0`, RVAs `0x044230`, `0x046950` and `0x0C6B80`, and the mount argument are not used in this range.

## 7. Write summary

| Selector | Targets written | Rest positions | Rate (effective) | Selector write | Flag byte |
|---|---|---|---|---|---|
| `0x19` | offsets, body Z (b1); creature offset X, body Z, creature angle Z, both hand angles, main offset (b3) | no | 0.01 | no | b3 |
| `0x1A` | as F (b1); body Z, both hand angles, main offset (b2); main angle, main offset (b3) | no | 0.01 / 0.03 / 0.01 | no | no |
| `0x1B` | as F | no | 0.01 | no | no |
| `0x1C` | body Z, creature angle Z, both hand angles | no | 0.01 / 0.2 / 0.01 | no | no |
| `0x1D` | hand angles, offsets, body angle, creature offset Y; hand turn (b3) | S | 0.01 / 0.04 / 0.01 | b3 → 0 | no |
| `0x1E` | hand angles, offsets, body Z, creature angle Z, creature offset X; head X (b2) | M | 0.04 / 0.05 / 0.03 | no | no |
| `0x1F` | as `0x1E` plus head Z and creature angle X | M | 0.05 / 0.05 / 0.03 | no | no |
| `0x20` | hand angles, offsets, body Z, creature angle Z or X, creature offset X (b1) | M | 0.04 / 0.05 / 0.03 | no | no |
| `0x21` | hand angles, offsets, body Z, creature offset Y | M | 0.02 / 0.05 / 0.03 | no | b2 |
| `0x22` | main angle, body Z | no | 0.01 | no | no |
| `0x23` | hand angles, offsets, body Z, creature offset Y (b1); hand turn (b3) | S (b3 only) | 0.005 / 0.05 / 0.01 | b3 → 0 | b1, b2 |
| `0x24` | none | no | 0.01 | no | no |

Here b1, b2 and b3 are the first, second and third branches of Section 5, and "offsets" means the hand-offset targets. Section 5 is authoritative; this table is a summary.

## 8. Random draws

None. The direct callees of these case bodies and shared blocks are RVAs `0x00EA90`, `0x011E30`, `0x028970`, `0x012280`, `0x051510`, `0x024B50`, `0x00E420`, `0x03CAA0`, `0x03D1A0`, `0x047310` and `0x044270`; no computed call occurs in them. The transitive closure of those routines in `calls.csv` adds only RVAs `0x047700`, `0x0478B0`, `0x044CB0`, `0x045FF0` and `0x0C7C00` and the imported precise cosine, sine and pow functions. It reaches neither `rand` nor `srand` and contains no computed call.

## 9. Server

`Server.exe` was not found to contain these routines. Three byte sequences taken from these case bodies each occur exactly once in `Cube.exe` and do not occur in `Server.exe`: the immediate stores of selector `0x22`, the `+0x1314` halving and modulo test of selector `0x1E`, and a run of immediate stores of selector `0x23`. The dispatch prologue of the evaluator also occurs once in the client and not in the server. This is consistent with the pose refresh being client-only, but a byte search is not a proof that no equivalent routine exists in another encoding.

## 10. Unresolved

- Whether code after the evaluator reads the scratch stack words of Section 1.2, in particular the final value p × 10 that selectors `0x1E` and `0x1F` leave in stack `+0x30`.
- The use of the off-hand auxiliary triplet (stack `+0x188`) and of the flag byte after the post-evaluator code; both consumers lie outside the held contract's sections.
- The meaning of owner `+0x1314` and owner `+0x64`, and the gameplay meaning of each selector.
- Behaviour for non-finite or zero helper results beyond the arithmetic stated (for example T1 or T2 equal to zero gives infinite or NaN ratios, clamped only when ordered greater than one), and ambient MXCSR and x87 state, denormals and imported math accuracy.
