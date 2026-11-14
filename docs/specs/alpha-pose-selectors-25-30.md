Status: FINAL / STABLE / HELD

# Alpha pose evaluator: selectors 0x25 through 0x30

This independent source-side contract closes the case bodies of the per-selector pose evaluator inside the client pose refresh for selectors `0x25` through `0x30` inclusive. The held contract `alpha-weapon-attachment-poses.md` (Section 4) leaves this evaluator bounded; that contract and `alpha-combat-helper-tables.md` were used only to locate the refresh, the dispatch, the blend that consumes the targets and the timing helpers. Every fact stated here was read again from the original. Field and slot names are storage descriptions only; no gameplay role is asserted.

## 1. Evidence

- Supported original `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (verified for this contract), preferred base `0x00400000`. `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` (verified), was used only for the absence check in Section 8.
- All addresses are RVAs. Instructions were read from `decompiled/client/assembly.txt`; the transitive call check used `decompiled/client/calls.csv`. The dispatch table and every constant held in image data were read from the image bytes through the section mapping. Constants encoded as instruction immediates are given with their bit patterns.
- The Infinite-Cubes reconstruction was not consulted for this contract. No decompiler output is relied on.

## 2. Terms and storage

- **C**: the pose subobject (the refresh's receiver, held in `EDI` throughout the cases). **Owner**: the creature pointed to by C's first word. "C `+x`" and "owner `+x`" are field offsets.
- **Clock**: signed 32-bit C `+0x4`. **Selector**: signed 32-bit C `+0x8`.
- **Slot `Lx`**: the 32-bit stack slot at `ESP + x` with `ESP` as it stands at the dispatch jump (RVA `0x013AF6`). Every case adjusts `ESP` only around calls, and each store below was converted to this base. A triplet slot `Lx` means `Lx`, `Lx+4`, `Lx+8` (components X, Y, Z). All slot values are binary32 unless stated.
- Timing helpers, each called on the owner (receiver = C's first word) with one stack argument equal to −1, which per `alpha-combat-helper-tables.md` selects the owner's action byte (their tables are closed there and not repeated; Section 8 re-confirms that they make no draws):
  - **B** = RVA `0x03CAA0`; **Cq** = RVA `0x047310`; **A** = RVA `0x044270`; **R0** = RVA `0x03D1A0` (no argument).
  - Each written evaluation is a separate call; the count and order of calls are stated per case. Every result is a signed 32-bit integer; sums and differences of these and the clock use 32-bit wrapping arithmetic, and every comparison with the clock is signed.
- **half(v)**: v divided by 2, truncated toward zero. **third(v)**: v divided by 3, truncated toward zero (exact for every signed 32-bit v; implemented by a multiply-high by `0x55555556` plus the sign bit).
- **f(v)**: binary32 conversion of a signed 32-bit integer with the packed integer conversion (exact up to magnitude 2^24, otherwise rounded under the ambient MXCSR mode).
- **Copy**: the triplet copy helper RVA `0x011E30` moves three 32-bit words (bit-exact, no floating-point operation). The element accessor RVA `0x028970` only returns the address of component i. The triplet builder RVA `0x00EA90` stores its three stack arguments as X, Y, Z by word moves.

### 2.1 Target and position slots

The slot-to-field mapping was read from the blend that follows the evaluator (RVAs `0x01E9D4–0x01EF76`) and from the final additions (RVAs `0x01F065–0x01F0A0`). Names follow the held poses contract.

| Slot | Role (field it drives) | Value at dispatch |
|---|---|---|
| `L134` | main-hand angle target (C `+0x30`) | zero |
| `L164` | off-hand angle target (C `+0x24`) | zero |
| `L14C` | main-hand offset target (C `+0x18`) | zero, or the bit-`0x400` values |
| `L170` | off-hand offset target (C `+0x0C`) | zero, or the bit-`0x400` values |
| `L140` | body-angle target (C `+0x54`) | zero |
| `L194` | creature-angle target (C `+0x60`) | zero, or (−70, 0, 0) |
| `L1A0` | head-angle target (C `+0x84`) | zero, or X = 70 |
| `L200` | tail-angle target (C `+0x90`) | zero |
| `L1AC` | creature-offset target (C `+0xAC`) | zero, or the bit-`0x400` Z |
| `L1B8` | target of the C `+0x78` triplet | zero, or (0, −1, 0) |
| `L1C4` | hand-turn target (C `+0x9C`) | zero |
| `L1F4` | body-offset target (C `+0x6C`) | zero |
| `L60` | lift target (C `+0xB8`) | as the held contract |
| `L74` | turn-scalar target (C `+0xA8`) | not audited |
| `L10` | blend rate | binary32 0.01 (bits `0x3C23D70A`, image `0x2FCDA0`) |
| `L158` | off-hand rest position (final value += C `+0x0C`) | rest position |
| `L17C` | main-hand rest position (final value += C `+0x18`) | rest position |

"zero" means positive zero stored by integer moves at RVAs `0x012FF3–0x013179`. "bit-`0x400` values" are those written at RVAs `0x013959–0x013ADB` when owner 16-bit `+0x124` has bit `0x400` set (re-read: `L194` = (−70, 0, 0), `L1A0` X = 70, `L1B8` = (0, −1, 0), `L1AC` Z = (owner `+0x84` − `+0x88`) × 0.5, both hand-offset targets (0, 3, 1) with the main-hand Y = 11 when owner byte `+0xAA9` = 7, then the conditional adjustment). The rest positions are those of the held contract, Section 3, stored at RVAs `0x0134E3–0x013743` (off hand in `L158`, main hand in `L17C`).

Further slots written by these cases (roles outside the held contract):

| Slot | Value at dispatch |
|---|---|
| `L188` (triplet) and `L23C` (triplet) | secondary positions computed at RVAs `0x013743–0x013804`; not closed here |
| `L1D0` (triplet) | owner `+0x100`, `+0x104`, `+0x108` (RVAs `0x013861–0x01388D`) |
| `L1DC` (triplet) | (−owner `+0x100` by sign toggle, owner `+0x104`, owner `+0x108`) |
| `L1E8` (triplet) | owner `+0xE8`, `+0xEC`, `+0xF0` (RVAs `0x012CA0–0x012CCA`) |
| `L218` (triplet) | owner `+0x10C`, `+0x110`, `+0x114` (RVAs `0x013896–0x0138E7`) |
| byte `L27` | 0 (RVA `0x013941`) |

Read-only slots used by selector `0x29`: `L14`, `L1C`, `L28` hold owner `+0xDC`, `+0xE0`, `+0xE4` (RVAs `0x012C5B–0x012C91`), written here P = (P.X, P.Y, P.Z); `L38`, `L94`, `L98` hold the same owner `+0x10C`, `+0x110`, `+0x114` copies as `L218`. No case in this range writes them.

After the evaluator (RVA `0x01E66D`, before the corrections) each secondary triplet is averaged: every component of `L188` becomes (`L158` + `L188`) × 0.5 and every component of `L23C` becomes (`L17C` + `L23C`) × 0.5 (binary32 sum, then binary32 product with 0.5). The later consumers of `L188`, `L23C`, `L1D0`, `L1DC`, `L1E8`, `L218` and `L27` lie after the weapon blocks of the held contract and are not closed here.

### 2.2 Shared tails

- **Tail T** (RVA `0x01E65A`): copies the triplet addressed by the case into a second triplet, then continues at the post-evaluator entry RVA `0x01E660`. Each use below says which triplet is copied to which.
- **Tail U** (RVA `0x01484F`, inside selector `0x27`'s range): sets `L128` = 0 and then `L124` = 0, then Tail T copying the scratch triplet `L120` into the tail-angle target `L200`. A jump to RVA `0x014844`, one instruction earlier, additionally sets the off-hand angle target X `L164` = −20 first.
- **Tail V** (RVA `0x014595`, inside selector `0x26`'s range): sets `L124` = −2 and `L128` = 0, then Tail T copying `L120` into `L1B8`.

`L120` is a scratch triplet; it is never read by the blend. "Exit" means a direct jump to RVA `0x01E660`.

## 3. Dispatch

At RVA `0x013AE9` the selector minus one is compared unsigned with `0x48`; above it jumps to RVA `0x01E660`, otherwise it jumps through the 73-entry table at image `0x023D4C` (entry index = selector − 1). Table entries for this range, read from the image bytes:

| Selector | Case RVA | Selector | Case RVA |
|---|---|---|---|
| `0x25` | `0x015DEE` | `0x2B` | `0x014878` |
| `0x26` | `0x0144FC` | `0x2C` | `0x014A6E` (also the entry for `0x49`) |
| `0x27` | `0x01466C` | `0x2D` | `0x01550E` |
| `0x28` | `0x015624` | `0x2E` | `0x015420` |
| `0x29` | `0x015771` | `0x2F` | `0x015332` |
| `0x2A` | `0x015113` | `0x30` | `0x0168AC` |

None of the entries `0x25`–`0x30` goes straight to the post-evaluator code. The entries that do (target RVA `0x01E660`) are those of selectors `0x0C`, `0x0D`, `0x0E`, `0x24` and `0x40`.

## 4. Properties common to the twelve cases

- They read no owner field directly except owner `+0x148`, `+0x14C`, `+0x150` (selector `0x30`); otherwise owner state enters only through B, Cq, A and R0. They read the clock, never write it, and do not read the mount argument, model pointers, the rest hand-angle slots, the weapon-scale slots or any matrix or series point.
- The only C field writes are: selector C `+0x8` = 0 (selectors `0x25` and `0x30`, one path each) and C `+0xBC`, `+0xCC`, `+0xD8`, `+0xD4` = 0 (selector `0x29`, one path). When the selector becomes zero, the post-evaluator entry also sets the clock to zero (RVA `0x01E666`), and later selector tests in the same refresh see zero.
- Calls made: B, Cq, A, R0, the copy helper, the element accessor and the triplet builder. No cosine, sine, square root or other imported math routine is called, and apart from C in `EDI` no case depends on a register value set before dispatch.
- Unless a path is said to write it, the blend-rate slot keeps its dispatch value 0.01.
- Every constant from image data is stated with its address; every target value written as an integer below is an exactly representable binary32 immediate (for example −90 = `0xC2B40000`, 110 = `0x42DC0000`, −1 = `0xBF800000`).

Bit patterns of the immediates used: 1 `0x3F800000`, 2 `0x40000000`, 3 `0x40400000`, 10 `0x41200000`, 20 `0x41A00000`, 30 `0x41F00000`, 40 `0x42200000`, 50 `0x42480000`, 60 `0x42700000`, 70 `0x428C0000`, 80 `0x42A00000`, 90 `0x42B40000`, 110 `0x42DC0000`, 200 `0x43480000`; the negative values have the sign bit set (−1 `0xBF800000`, −2 `0xC0000000`, −3 `0xC0400000`, −4 `0xC0800000`, −6 `0xC0C00000`, −10 `0xC1200000`, −20 `0xC1A00000`, −30 `0xC1F00000`, −60 `0xC2700000`, −70 `0xC28C0000`, −80 `0xC2A00000`, −90 `0xC2B40000`). Zero is integer 0 (positive zero).

Image constants (binary32): 0.002 `0x3B03126F` (`0x2FCD98`), 0.005 `0x3BA3D70A` (`0x2FCD9C`), 0.01 `0x3C23D70A` (`0x2FCDA0`), 0.02 `0x3CA3D70A` (`0x2FCDA8`), 0.05 `0x3D4CCCCD` (`0x2FCDB4`), +0 `0x00000000` (`0x345D84`), 1 (`0x345DC0`), 2 (`0x345E10`), 4 (`0x345E60`), 8 (`0x345E70`), 10 (`0x345E74`), −30 (`0x345E9C`), 90 (`0x2FCE54`), 360 (`0x2FCE6C`), −80 (`0x2FCEAC`), −90 (`0x2FCEB0`), −360 (`0x2FCEBC`).

In the per-case tables, "X = v" means only that component is written; triplets in parentheses write all three components.

## 5. Selectors `0x25`–`0x28`

### 5.1 Selector `0x25` (RVAs `0x015DEE–0x016052`)

Decision order (first true row):

| Phase | Condition | Writes, in order |
|---|---|---|
| 1 | clock < B | `L134` = (−90, −90, 80); `L148` (body-angle Z) = −80. Scratch s1 = `L18C`, s2 = `L190`. |
| 2 | clock < R0 (R0 evaluated only when phase 1 fails) | `L134` = (−90, 0, −90); `L17C` = `L17C` + 4; `L148` = 90; byte `L27` = 1; `L1E0` = `L1E0` − 4; `L1D4` = `L1D4` + 4; rate = 0.05 (`0x2FCDB4`). s1 = `L18C`, s2 = `L190`. |
| 3 | otherwise | selector = 0; `L180` = `L180` + 4; `L17C` = `L17C` − 8; `L134` X = −60, `L13C` (Z) = 20 (`L138` keeps its dispatch value); `L148` = −20; s1 = `L18C` + 10; s2 = `L190` − 4. Phase 3 also copies `L17C` into `L158` and then subtracts 2 from `L158` and 1 from `L160`; these writes are overwritten by the common step and are not observable. |

Common step (all phases, in order): `L158` = copy of `L17C`; `L158` = `L158` + 2; `L18C` = s1 + 10; `L190` = s2 − 4; Tail T copies `L134` into `L164`.

So `L18C` ends as `L18C` + 10 (phases 1–2) or (`L18C` + 10) + 10 (phase 3), and `L190` as `L190` − 4 or (`L190` − 4) − 4, each addition a separate binary32 rounding. The off-hand rest position ends equal to the main-hand rest position (after any phase 2 or 3 change) with X increased by 2, and the off-hand angle target equals the main-hand angle target. Effective rate: 0.01 in phases 1 and 3, 0.05 in phase 2. Calls: B once; R0 once in phases 2 and 3.

s1 and s2 are held in slots `L18` and `L7C`. The post-evaluator code also addresses slots at these offsets; whether it reads these values before rewriting them was not audited (Section 10).

### 5.2 Selector `0x26` (RVAs `0x0144FC–0x01466C`)

Threshold H = B + half(Cq), with Cq evaluated first and then B.

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < H | `L1A0` = (−30, 10, 90); `L140` = (0, 0, 60); Tail T: `L1B8` = (−3, −2, 0) |
| 2 | clock ≥ H and clock < R0 | `L1A0` = (30, −10, −30); `L140` = (10, 0, −20); Tail T: `L1B8` = (3, 0, 2) |
| — | otherwise | exit, no writes |

Rate 0.01. No selector write.

### 5.3 Selector `0x27` (RVAs `0x01466C–0x014878`)

H as for `0x26` (Cq, then B).

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < H | `L1A0` = (−70, 0, 0); `L140` = (−10, 0, 0); `L1B8` = (0, −3, −1); `L164` X = 20; `L134` X = 20; exit |
| 2 | clock < R0 | `L1A0` = (50, 0, 0); `L140` = (20, 0, 0); `L1F4` = (0, −3, 1); `L1B8` = (0, 0, 2); `L134` X = −20; `L164` X = −20; Tail T: `L200` = (−10, 0, 0) |
| — | otherwise | exit, no writes |

Rate 0.01.

### 5.4 Selector `0x28` (RVAs `0x015624–0x015771`)

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < B | `L19C` (creature-angle Z) = 90; rate = 0.002 (`0x2FCD98`); `L1A0` = (0, 10, −60); Tail T: `L200` = (0, 0, −70) |
| 2 | clock < R0 | `L19C` = 200; `L1A0` = (0, −20, 60); `L1B8` = (−3, 0, −2); Tail T: `L200` = (0, 0, 70) |
| — | otherwise | exit, no writes |

Effective rate: 0.002 in phase 1, otherwise 0.01.

## 6. Selectors `0x29`–`0x2C`

### 6.1 Selector `0x29` (RVAs `0x015771–0x015DEE`)

The clock is read once at entry (value k), then B is evaluated (B1).

**Phase 1, k < B1.** B is evaluated again (B2) and t = f(k) ÷ f(B2) (binary32 division, no clamp). Then each field F below is set to ((G − F) × t) + F, with one rounding per operation, where G is its goal:

| Fields F | Goals G |
|---|---|
| `L1E8`, `L1EC`, `L1F0` | P.X − (+0), P.Y − (+0), P.Z − 2 |
| `L158`, `L15C`, `L160` | P.X, P.Y, P.Z |
| `L17C`, `L180`, `L184` | P.X, P.Y, P.Z |
| `L1DC`, `L1E0`, `L1E4` | P.X, P.Y, P.Z |
| `L1D0`, `L1D4`, `L1D8` | P.X, P.Y, P.Z |
| `L218`, `L21C`, `L220` | with base values `L38`, `L94`, `L98` in place of the old F: `L218` = ((P.X + (+0)) − `L38`) × t + `L38`; `L21C` = ((P.Y + 4) − `L94`) × t + `L94`; `L220` = ((P.Z + 4) − `L98`) × t + `L98` |

Subtracting positive zero leaves every value unchanged; adding positive zero turns a negative zero into positive zero. All goals are formed from the read-only slots, so the store order (which the original interleaves) has no effect. Exit.

**Phase 2, k ≥ B1.** B is evaluated (B2), then Cq. If the clock < B2 + Cq: C `+0xBC`, `+0xCC`, `+0xD8`, `+0xD4` are each set to integer 0 (in that order); Tail T: `L1F4` = (0, 0, −3).

**Phase 3, otherwise.** If the clock ≥ R0, exit with no writes. Otherwise B (B3), Cq and A are evaluated in that order; d = (clock − B3) − Cq; u = f(d) ÷ f(A); w = 1 − u (binary32, no clamp). The same field set and goals as phase 1 are updated with w in place of t: F = ((G − F) × w) + F, and the `L218` triplet uses the bases `L38`, `L94`, `L98` as in phase 1. Exit.

No rate or selector write. Calls: phase 1 B twice; phase 2 B twice and Cq once; phase 3 B three times, Cq twice, R0 once and A once (the phase 2 calls precede the phase 3 test).

### 6.2 Selector `0x2A` (RVAs `0x015113–0x015332`)

H = B + half(Cq), Cq evaluated first.

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < H | `L1A0` = (−10, 10, 30); `L140` = (−10, 0, 0); `L1B8` = (−3, 0, −1); `L164` X = 20; `L134` X = 20; rate = 0.005 (`0x2FCD9C`); exit |
| 2 | clock < R0 | `L1A0` = (50, 30, 0); then B and Cq are evaluated (B first) and, if the clock < B + Cq, `L1A4` (head Y) = −30; then `L140` = (30, 0, 0); `L1F4` = (0, −1, 2); `L1B8` = (0, 0, 2); `L134` X = −60; rate = 0.005; `L120` X = −20; Tail U from RVA `0x014844`: `L164` X = −20, `L200` = (−20, 0, 0) |
| — | otherwise | exit, no writes |

Effective rate 0.005 in phases 1 and 2.

### 6.3 Selector `0x2B` (RVAs `0x014878–0x014A6E`)

H = B + half(Cq), Cq first.

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < H | `L1A0` = (−20, 0, 0); `L140` = (50, 0, 0); `L1F4` = (0, −2, 2); `L1B8` = (0, −6, −2); `L164` X = 20; `L134` X = 30; `L120` X = −50; Tail U: `L200` = (−50, 0, 0) |
| 2 | clock < R0 | `L1A0` = (−10, 0, 0); `L140` = (0, 0, 0); `L1B8` = (0, 0, 1); `L164` X = 0; `L134` X = 0; rate = 0.02 (`0x2FCDA8`); exit |
| — | otherwise | exit, no writes |

Effective rate 0.02 in phase 2, otherwise 0.01.

### 6.4 Selector `0x2C` and its alias `0x49` (RVAs `0x014A6E–0x015113`)

Five phases, tested in order. Thresholds: H1 = B; H2 = B + third(Cq); H3 = B + third(2 × Cq) with the doubling in wrapping 32-bit arithmetic; H4 = B + Cq; H5 = R0. Each threshold is evaluated only when the earlier ones failed; for H2 and H3 Cq is called before B, for H4 B before Cq.

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < H1 | `L1A0` = (10, 0, 0); `L140` = (−20, 0, 0); `L1F4` = (0, −1, 0); `L1B8` = (0, 1, 1); `L164` X = 20; `L134` X = 20; `L178` (off-hand offset Z) = 2; `L154` (main-hand offset Z) = 2; Tail T: `L200` = (30, 0, 10) |
| 2 | clock < H2 | `L1A0` = (−30, 0, 0); `L140` = (50, 0, 0); `L1F4` = (0, 0, 1); `L1B8` = (0, −4, 2); `L164` X = 60; `L134` X = 70; `L120` X = −30; Tail U: `L200` = (−30, 0, 0) |
| 3 | clock < H3 | `L1A0` = (−30, 0, 0); `L140` = (40, 0, 0); `L1F4` = (0, 1, 1); `L1B8` = (0, −2, 2); `L164` X = 70; `L134` X = −30; `L14C` = (0, 3, −2); Tail T: `L200` = (−30, 0, −10) |
| 4 | clock < H4 | `L1A0` = (−30, 0, 0); `L140` = (10, 0, 0); `L1F4` = (0, 1, 1); `L1B8` = (0, −2, 2); `L164` X = −30; `L134` X = −40; `L170` = (0, 3, −2); Tail T: `L200` = (−30, 0, −10) |
| 5 | clock < H5 | `L1A0` = (10, 0, −10); `L140` = (−20, 0, 0); `L1F4` = (0, 1, 0); `L1B8` = (0, 1, 1); `L164` X = 20; `L134` X = 20; `L178` = 2; `L154` = 2; Tail T: `L200` = (30, 0, 10) |
| — | otherwise | exit, no writes |

Rate 0.01 throughout; no selector write. Selector `0x49` executes exactly this body.

## 7. Selectors `0x2D`–`0x30`

### 7.1 Selector `0x2D` (RVAs `0x01550E–0x015624`)

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < B (B1) | `L140` X = −10; `L1A0` X = 10; `L200` X = −60; exit |
| 2 | clock < B2 + Cq1 (B evaluated, then Cq) | B (B3) and Cq (Cq2) evaluated in that order; v = (f(clock − B3) ÷ f(Cq2)) × −360 (`0x2FCEBC`), binary32 division then product, no clamp; `L164` X = 80; `L134` X = 80; `L1A0` X = −60; `L194` (creature-angle X) = v; Tail T: `L1B8` = (0, −3, 0) |
| 3 | clock < R0 | `L140` X = 20; `L1A0` X = −10; `L120` X = 0; Tail V: `L1B8` = (0, −2, 0) |
| — | otherwise | exit, no writes |

Rate 0.01.

### 7.2 Selector `0x2E` (RVAs `0x015420–0x01550E`)

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < B | `L148` = −10; `L144` = 30; `L168` (off-hand angle Y) = −30; `L138` (main-hand angle Y) = −30; `L1A8` (head Z) = 60; exit |
| 2 | clock < B2 + Cq1 | B3 then Cq2; v = (f(clock − B3) ÷ f(Cq2)) × −360 (`0x2FCEBC`); `L168` = 80; `L138` = 80; `L198` (creature-angle Y) = v; exit |
| 3 | clock < R0 | `L144` = −30; `L168` = 30; `L138` = 30; exit |
| — | otherwise | exit, no writes |

Rate 0.01.

### 7.3 Selector `0x2F` (RVAs `0x015332–0x015420`)

The same structure and call order as `0x2E` with mirrored values:

| Phase | Condition | Writes |
|---|---|---|
| 1 | clock < B | `L148` = 10; `L144` = −30; `L168` = 30; `L138` = 30; `L1A8` = −60; exit |
| 2 | clock < B2 + Cq1 | v = (f(clock − B3) ÷ f(Cq2)) × 360 (`0x2FCE6C`); `L168` = −80; `L138` = −80; `L198` = v; exit |
| 3 | clock < R0 | `L144` = 30; `L168` = −30; `L138` = −30; exit |
| — | otherwise | exit, no writes |

Rate 0.01.

### 7.4 Selector `0x30` (RVAs `0x0168AC–0x016D25`)

First, on every path: the hand-turn target `L1C4` = copy of owner `+0x148`, `+0x14C`, `+0x150`. Then B is evaluated (B1) and h1 = half(B1).

| Phase | Condition | Writes, in order |
|---|---|---|
| 1 | clock < h1 | B evaluated again (B2), h2 = half(B2); s = f(clock) ÷ f(h2), then s = 1 if s > 1 (ordered; NaN is kept); `L144` = s × 10; `L148` = s × −80 (`0x2FCEAC`); with m = 1 − s (`0x345DC0`): `L134` = ((m × −90) + (s × −90), (m × (+0)) + (s × 90), (m × (+0)) + (s × −30)) using −90 `0x2FCEB0`, +0 `0x345D84`, 90 `0x2FCE54`, −30 `0x345E9C`; then step R |
| 2 | clock < B2 (B evaluated again) | `L140` = (10, 0, −90); `L134` = (0, 110, −20); then step R with `L158` X + 2 |
| 3 | clock < R0 | `L19C` = 20 (later overwritten); `L140` = (0, 0, 90) (X, Y, Z written in that order); `L134` = (0, 90, 90) via the triplet builder; then step R with `L158` X − 2 |
| 4 | otherwise | selector = 0; `L158` = copy of `L17C`; `L158` X = `L158` X − 2; `L160` = `L160` − 1; `L18C` = `L18C` + 10; `L190` = `L190` − 4; `L138` = 90; `L148` = −80; `L164` = copy of `L134`; then step W |

Step R (phases 1–3): `L158` = copy of `L17C`; `L158` X = `L158` X + 2 (phases 1 and 2) or − 2 (phase 3); `L18C` = `L18C` + 10; `L190` = `L190` − 4; rate = 0.01 (`0x2FCDA0`, the same as the dispatch value); then step W.

Step W (RVA `0x016C9F`, all phases): B is evaluated; e = f(clock − B); Cq is evaluated; r = e ÷ f(Cq); r = 1 if r > 1 (ordered), else r = +0 if r < 0 (ordered); NaN and negative zero pass unchanged; `L19C` (creature-angle Z) = r × 360 (`0x2FCE6C`); Tail T copies `L134` into `L164`.

In phase 1 the products m × (+0) are positive zero for finite m ≥ 0 and NaN when m is infinite or NaN; all three components are stated as computed, with one rounding per product and per sum. Effective rate 0.01 on every path. Calls: B two times (phase 1 or 2 test) or two times plus R0 (phases 3, 4), plus one B and one Cq in step W.

## 8. Random draws and the server

The transitive direct-call closure (from `calls.csv`) of every routine these case bodies call — B, Cq, A, R0, RVAs `0x011E30`, `0x028970`, `0x00EA90` — reaches only RVAs `0x044CB0`, `0x045FF0`, `0x047700`, `0x0478B0`, `0x0C7C00` and the imported precise power routine, with no computed call; it reaches neither the imported `rand` nor `srand`. The case bodies contain no other call. These selectors therefore make no random draws.

`Server.exe` was byte-scanned for five address-free runs of stack-slot immediate stores taken from these case bodies (starting at RVAs `0x01542E`, `0x015340`, `0x01551C`, `0x014B54`, `0x015F71`, 33 to 76 bytes) and for the dispatch sequence at RVA `0x013AE9` (which contains a relative branch and the table's absolute address, so it is the weaker signature). Each occurs in the client (once, or twice for `0x014B54`, which selector `0x2C` repeats in its last phase) and none occurs in the server image. No evidence was found that the server contains this evaluator.

## 9. Relation to the held poses contract

These closures replace, for selectors `0x25`–`0x30` and `0x49`, the bounded description of Section 4 of `alpha-weapon-attachment-poses.md`. The post-evaluator averaging of `L188` and `L23C` (Section 2.1) is not described in that contract; it is stated here as read from RVAs `0x01E66D–0x01E6EC`. Selector-dependent code outside the evaluator (arm swing, hand matrix, series branches) is outside this contract.

## 10. Unresolved

- The roles and later consumers of the secondary triplets `L188` and `L23C`, of `L1D0`, `L1DC`, `L1E8`, `L218`, the flag byte `L27`, the turn-scalar target's dispatch value, and of C `+0xBC`, `+0xCC`, `+0xD4`, `+0xD8`.
- Whether the scratch values selector `0x25` leaves in slots `L18` and `L7C` are read by later code of the refresh before being rewritten (the post-evaluator code addresses those offsets at stack depths not audited here).
- Behaviour when a helper result used as a divisor is zero or a sum wraps (the binary32 quotient is then infinite or NaN and propagates as computed); ambient MXCSR state; non-finite owner inputs.
- The meaning of the selector values and of the timing phases.
