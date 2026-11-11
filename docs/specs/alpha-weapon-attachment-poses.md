Status: FINAL / STABLE / HELD

# Alpha weapon attachment poses: producing chain, branches, initial state and frame freshness

This independent source-side contract states how the supported Alpha client produces the two creature weapon attachment points at creature `+0x1A04` (called A) and `+0x1B84` (called B), from the inputs of one pose refresh through the stored triplets, and when those stores happen relative to the frame's world update and to the renderer's light and geometry reads. It bounds the large per-action pose evaluator by its inputs and outputs. Field names are storage descriptions only; no gameplay role is asserted beyond what the arithmetic shows.

The supported original is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (hash verified for this contract), preferred base `0x00400000`. All addresses are RVAs. Evidence paths are relative to the original evidence root. The pose refresh is RVA `0x0128F0`; its listed body is `decompiled/client/assembly-functions/004128f0.asm` (ranges `0x0128F0–0x013AFC` and `0x01E660–0x023D48`). Its per-action case bodies (`0x013AFC–0x01E660`) and all helpers were read from `decompiled/client/assembly.txt`. The renderer is RVA `0x0AC260`, `decompiled/client/assembly-functions/004ac260.asm`. Constants were read from the image bytes. Decompiler exports were used only as leads. The earlier contract `alpha-creature-light-attachment-producers.md` was used only to locate members; every fact relied on here was re-confirmed in the original. The Infinite-Cubes alpha reconstruction was consulted for leads about member layout and ordering; every such lead used below was confirmed against original instructions, and nothing is stated on its authority alone.

## 1. Storage used by the chain

The pose subobject C starts at creature `+0x1498`; its first word points to the owning creature. In this contract "owner `+x`" means a creature field and "C `+x`" a subobject field (creature offset = C offset + `0x1498`). All triplets are three consecutive binary32 values unless stated.

| C offset (creature) | Representation and role in this chain |
|---|---|
| `+0x4` (`+0x149C`) | Signed 32-bit pose clock |
| `+0x8` (`+0x14A0`) | Signed 32-bit pose selector |
| `+0xC`, `+0x18` | Blended off-hand and main-hand offset triplets |
| `+0x24`, `+0x30` | Blended off-hand and main-hand angle triplets (degrees) |
| `+0x54`, `+0x60`, `+0x6C` | Blended body angles, creature angles, body offset |
| `+0x9C`, `+0xA8`, `+0xAC`, `+0xB8` | Blended hand-turn triplet, a turn scalar, creature offset triplet, lift scalar |
| `+0xDC`, `+0xE0` | Main-hand and off-hand weapon model pointers (null allowed) |
| `+0xEC`, `+0x12C` | Body and hand 4-by-4 binary32 matrices |
| `+0x2AC`, `+0x2EC` | Main-hand and off-hand weapon matrices |
| `+0x56C` (`+0x1A04`) | A: first point of a 16-point series S0 (12-byte stride) |
| `+0x62C` (`+0x1AC4`) | First point of series S1 |
| `+0x6EC` (`+0x1B84`) | B: first point of series S2 |
| `+0x7AC` (`+0x1C44`) | First point of series S3 |
| `+0x880`, `+0x888` | Copied signed 64-bit horizontal references |

The four series occupy C `+0x56C` through `+0x86B` contiguously, 16 triplets each. Matrices use element index row + 4 × column (column-major), so column 3 holds translation. Weapon model dimensions are the signed 32-bit words at model `+0x44`, `+0x48`, `+0x4C`, written here sx, sy, sz (getters RVAs `0x024810`, `0x024820`, `0x07B5F0`). Every use converts the signed 32-bit integer to binary32 with the packed integer conversion (exact up to magnitude 2^24, otherwise rounded under the ambient MXCSR mode). In this contract "image `X`" names data at image RVA `X`.

## 2. Numeric primitives

All arithmetic below is scalar binary32 unless a binary64 step is named; there are no fused operations. Products and two-operand sums are commutative in IEEE arithmetic, so only grouping is stated.

- Translate by (x, y, z), RVA `0x024A60` (vector form RVA `0x024990`, same arithmetic): for each row r, new column 3 = ((col0·x + col1·y) + col2·z) + col3. Columns 0–2 are unchanged.
- Scale by (x, y, z), RVA `0x024730`: column 0 is multiplied by x, column 1 by y, column 2 by z, each element separately. A column is skipped only when its factor compares ordered-equal to binary32 one (bits `0x3F800000`, image `0x345DC0`); an unordered factor is applied.
- Axis rotations about Z, Y and X, RVAs `0x024610`, `0x0244F0`, `0x0243D0`: the degree angle is multiplied in binary32 by bits `0x3C8EFA35` (image `0x2FCDA4`), the rounded radian value is widened to binary64 for the imported MSVCR110 precise SSE2 cosine and then sine; each result is narrowed to binary32 (c, s). For each row: Z rotation sets col0 = col0·c + col1·s and col1 = col1·c − col0·s; Y rotation sets col0 = col0·c − col2·s and col2 = col2·c + col0·s; X rotation sets col1 = col1·c + col2·s and col2 = col2·c − col1·s, all using the pre-update values.
- Axis-angle rotation by angle a about (x, y, z), RVA `0x0241B0`: L = binary32 narrowing of the imported binary64 square root of the widened ((x·x + y·y) + z·z). If L compares ordered-equal to zero the matrix is unchanged. Otherwise c and s are formed as above, the axis is divided by L component-wise (x', y', z'), t = 1 − c, and a rotation matrix R is built with diagonal (x'·x')·t + c, (y'·y')·t + c, (z'·z')·t + c; element 1 = t·(x'·y') + s·z', element 4 = t·(x'·y') − s·z'; element 2 = t·(x'·z') − s·y', element 8 = t·(x'·z') + s·y'; element 6 = t·(y'·z') + s·x', element 9 = t·(y'·z') − s·x'; elements 3, 7, 11, 12, 13, 14 zero and 15 one. The current matrix M is replaced by M·R through RVA `0x012400`, where each new element of row r, column k is ((M[r,0]·R[0,k] + M[r,1]·R[1,k]) + M[r,2]·R[2,k]) + M[r,3]·R[3,k], each row read before it is written. A zero angle about a unit axis still performs this multiply (identity up to signed-zero and non-finite propagation).
- Point transform, RVA `0x0248A0`: for each output row r ∈ {0,1,2}, numerator = ((M[r,0]·x + M[r,1]·y) + M[r,2]·z) + M[r,3]; denominator w = ((M[3,0]·x + M[3,1]·y) + M[3,2]·z) + M[3,3]; reciprocal = 1 / w; output = reciprocal × numerator, stored X, Y, Z. Every matrix built in this chain keeps row 3 equal to (0, 0, 0, 1), so for finite inputs w is exactly one and the outputs equal the numerators.
- Fixed-to-float: a signed 64-bit value is loaded exactly by the x87 unit, rounded to binary32 at the store under the ambient x87 rounding control, then multiplied in binary32 by `2^-16` (bits `0x37800000`, image `0x2FCD94`) (RVA `0x0120F0`). Float-to-fixed: binary32 × `65536` (bits `0x47800000`, image `0x2FCE7C`), then the runtime conversion RVA `0x28D946` to signed 64-bit (RVAs `0x012220`, `0x0122E0`).
- Repeated approach, RVA `0x011D40`: starting from a value, each of n repeats sets value = ((target − value) × rate) + value with a store per repeat; n ≤ 0 performs nothing.

## 3. Refresh entry and pre-evaluator inputs

Entry, stack arguments 1–10: shader, view, projection, model collection, signed 32-bit repeat count n, color, pointers to reference X and reference Y, a part mask, and a mount pointer. On entry the routine stores byte one at C `+0x878`, copies the two references into C `+0x880` and `+0x888`, clears C `+0xBC`, `+0xC0`, `+0xC8`, and looks up model pointers through RVA `0x0EC400`, among them C `+0xDC` from the owner record at `+0xAA8` and C `+0xE0` from the record at `+0x990` (RVAs `0x012910–0x012A1D`).

Rest hand angles (RVAs `0x012D15–0x012DDA`): two local angle triplets start at zero. Only when the selector C `+0x8` equals zero, the main-hand triplet becomes owner `+0xC4`, `+0xC8`, `+0xCC` and the off-hand triplet becomes the same values with components 1 and 2 sign-flipped (bit-exact sign toggles). Two further local triplets, one per hand, are zeroed at RVAs `0x012CD3–0x012D0A`; no instruction in the routine or its case bodies writes them afterwards, so the per-hand extra turns used in Sections 8 and 9 are always zero.

Weapon scales: two locals both copy owner binary32 `+0xB0` (RVAs `0x012C1F–0x012C30`), one per hand.

Arm swing (RVAs `0x012DE3–0x012FF3`): cycle = binary32 narrowing of the imported sine of the widened binary32 narrowing of (widened owner `+0x118C` + binary64 pi/2, bits `0x3FF921FB54442D18`, image `0x2FCDF0`). The swing q = narrowing of ((widened owner `+0x1188`) × binary64 bits `0x400197C982C286E6`, image `0x2FCDF8`) × widened cycle, in binary64. Then, in order: owner byte `+0x68` equal to `0x4F` multiplies q by 0.5; `+0x68` equal to `0x6A` or `0x6B` multiplies by 0.1 (bits `0x3DCCCCCD`); a lean condition multiplies by 1.25 and sets a lean flag; owner 16-bit `+0x7E` bit 1 set with owner byte `+0x5C` bit 0 clear sets q to positive zero; `+0x7E` bit 0 multiplies by 0.75. The value at this point is kept separately as the bob source u. Then owner byte `+0xAA9` equal to 5 with `+0x68` nonzero multiplies by 0.25; selector `0x33` or `0x34` with owner signed `+0x6C` less than the result of RVA `0x0C6B80` on owner `+0x1E8` multiplies by 0.1; selector `0x35` multiplies by 0.1; `+0x68` in {`0x16`, `0x24`, `0x3F`, `0x08`, `0x1C`} or selector in {2, 3, `0x30`, `0x31`, `0x25`} multiplies by 0.5; owner 16-bit `+0x124` bit `0x10` sets q to positive zero. The lean condition holds when `+0x68` equals `0x30`, or when not (`+0x124` bit 0 and `+0x5C` bit 2), `+0x124` bit `0x40` is set, ((owner `+0x40`)² + (`+0x44`)²) + (`+0x48`)² > 0.1, and (`+0x38`)² + (`+0x34`)² > 0.1, all binary32 ordered comparisons.

Rest hand positions (RVAs `0x0134E3–0x013743`), with h = owner `+0xF4`, `+0xF8`, `+0xFC` and W = owner `+0x1188`:
- If `+0x7E` bit 0 is set: main hand = (h0, h1, h2) and off hand = (−h0, h1, h2).
- Otherwise R = (h0·W) + h0; c and s are binary32 narrowings of the imported cosine and sine of widened q; X = −(c·R) for the off hand and c·R for the main hand; the off-hand Y = h1 + (−(s·R)) and the main-hand Y = h1 + (s·R); both Z = (h2 − 4.1) + (W·6), using bits `0x40833333` (image `0x2FCE20`) and `0x40C00000`.
- Then both Y become (2.1 − (W·5)) + Y (bits `0x40066666`, image `0x2FCE00`).
- If owner 64-bit `+0x11C0` is nonzero, both Y have 2 added and both X are multiplied by 0.5.

Owner off-hand item category byte `+0x990` equal to 3 with kind `+0x991` equal to `0x0E` clears C `+0xE0` before the evaluator (RVAs `0x013898–0x01390F`), so B is not freshly produced in that case (Section 9).

Pose targets: target triplets for the four hand fields, body angles, creature angles, creature offset, hand turn, body offset, head and tail fields are zeroed; the lift target starts at zero and becomes owner `+0x88` × 0.5 when predicate RVA `0x044650` is true (owner `+0x124` bit `0x10`, owner `+0x3C` ordered less than zero, `+0x5C` bit 0 clear, signed `+0x12C` ≤ 0 and signed `+0x128` ≤ 0). The blend rate starts at binary32 0.01 (bits `0x3C23D70A`). If owner `+0x124` bit `0x400` is set: creature-angle target (−70, 0, 0), both hand-offset targets (0, 3, 1), the main-hand offset target Y becomes 11 when `+0xAA9` equals 7, the creature-offset target Z becomes (owner `+0x84` − owner `+0x88`) × 0.5, and, when `+0x68` is nonzero and `+0x6C` is less than RVA `0x03D1A0`'s result, each hand-offset target has 0 added to X, 5 subtracted from Y and 6 added to Z (RVAs `0x013959–0x013ADB`).

## 4. The per-selector evaluator (bounded)

Selectors 1 through `0x49` dispatch through the 73-entry table at image `0x023D4C` into case bodies at `0x013AFC–0x01E660`; selector 0, negative selectors and selectors above `0x49` bypass it, and table entries for `0x0C`, `0x0D`, `0x0E`, `0x24`, `0x40` go straight to the post-evaluator code. Observable bounds from the original case bodies: they read owner state, the pose clock, owner timing helpers RVAs `0x03CAA0`, `0x047310`, `0x03D1A0`, `0x044230`, `0x044270`, `0x046950`, `0x0C6B80`, model lookup RVA `0x0120C0`, and imported cosine and sine through wrappers RVAs `0x00E420` and `0x024B50`; they store into the target locals, the two rest hand-position locals and, at a number of sites, the stack slot of the blend rate (stack depth at each case store was not individually audited), and some set the selector C `+0x8` to zero. They contain no reference to the rest hand-angle locals, the arm-swing value, the zero turn locals, the weapon scale locals (one read only), any part matrix, or any series point, and they do not write the pose clock. The mount argument is read only inside the selector `0x15` case. The individual case arithmetic is not closed by this contract; for a nonzero selector, the hand positions and targets after the evaluator are bounded outputs.

## 5. Post-evaluator corrections and blend

At RVA `0x01E660`, a zero selector also zeroes the pose clock. Corrections, in order (RVAs `0x01E6F1–0x01E9D0`):
- Owner signed `+0x12C` > 0: creature-angle target becomes (0, 90, 0) with creature-offset target Z = (owner `+0x80` − `+0x88`) × 0.5 when `+0x7E` bit 0 is set, otherwise (90, 0, 0) with Z = (`+0x84` − `+0x88`) × 0.5.
- `+0x124` bit 0 and `+0x5C` bit 2: both hand-offset targets Z += 7 and both hand-angle targets X += 90.
- `+0x991` equal to `0x14`: off-hand angle target X = 0; off-hand offset target Y += 4 and Z += 4.
- A lean correction changes only creature-angle and head targets.

The blend amount g starts at binary32 zero and approaches one at the rate local, repeated n times (RVA `0x011D40`). Each linear field F with target T becomes ((T − F) × g) + F component-wise, in this order: C `+0x30`, `+0x24`, `+0x18`, `+0x0C`, `+0x54`, `+0xAC`, then lift `+0xB8`. The creature angles C `+0x60`, `+0x64`, `+0x68` (then head and tail angles) use the angle-approach helper RVA `0x023F70`, re-read in the original: each angle is divided in binary32 by 180, widened, multiplied by binary64 pi and narrowed; imported cosine and sine of the current and target angles are narrowed and re-widened; the binary64 value (target sine × current cosine) − (target cosine × current sine) is narrowed and clamped to [−1, 1] when ordered (NaN passes); the imported arcsine is narrowed and re-widened; the increment is the narrowing of ((widened g × binary64 180) ÷ pi) × arcsine, added in binary32 to the current angle. Then the turn scalar C `+0xA8` and the triplets C `+0x9C`, `+0x6C` and `+0x78` blend linearly (RVAs `0x01E9D4–0x01EF76`).

Finally (RVAs `0x01F065–0x01F0A0`) the main-hand angle local += C `+0x30`, the off-hand angle local += C `+0x24`, the main-hand position += C `+0x18`, the off-hand position += C `+0x0C`. Part-mask bit `0x40` then clears both weapon model pointers (RVA `0x01F12F`). With n = 0, g is exactly zero and no blended field moves.

## 6. Creature matrix

Built in a local matrix (RVAs `0x01F154–0x01F5C7`), in this exact order:
1. Identity.
2. Translate by fixed-to-float of (P.X + reference X), of (P.Y + reference Y), and of ((P.Z − float-to-fixed(owner `+0x1180`)) + float-to-fixed(C `+0xB4`)), with 64-bit integer sums; P is the signed 64-bit triplet at owner `+0x1350`.
3. Roll, only when owner signed `+0x128` is nonzero and not (owner byte `+0x140` = 4, `+0x141` = 1, `+0x6C` < RVA `0x03D1A0`'s result and `+0x68` ∈ {`0x11`, 5, `0x14`}): angle = (1 − (binary32 of `+0x128`) ÷ 600) × −360; axis = cross product of owner velocity `+0x34` with (0, 0, 1) by RVA `0x012390`, replaced by the velocity itself when owner signed `+0x64` equals `0x65`; if ((x² + y²) + z²) > 0.01 the axis is normalized by RVA `0x0240F0` (each component times 1 ÷ narrowed square root) and an axis-angle rotation applied. In this branch, whether or not rotated, both hand-position Z values receive += (owner `+0x88` × 5) ÷ owner `+0x80`.
4. If `+0x124` bit `0x10`: both hand Z values again += (`+0x88` × 5) ÷ `+0x80`.
5. Z rotation by owner `+0x137C`; translate (0, 0, C `+0xB8`); Y rotation by owner `+0x1378`; X rotation by owner `+0x1374`.
6. Translate (C `+0xAC`, C `+0xB0`, 0); Z rotation by C `+0x68`, Y rotation by C `+0x64`, X rotation by C `+0x60`.
7. Translate (0, 0, −C `+0xB8`) with an exact sign toggle; scale uniformly by owner `+0x80` ÷ 11.2 (bits `0x41333333`, image `0x2FCE2C`).

The bob value b = |u| × 4 (absolute value by clearing the sign bit, exact), times 4 again when `+0x68` equals `0x4F` (RVAs `0x01F5CC–0x01F60B`).

## 7. Body and hand matrices

Body matrix C `+0xEC` (RVAs `0x01F611–0x01F71B`): copy of the creature matrix; translate (0, 0, b); translate (C `+0x6C`, `+0x70`, `+0x74`); Z rotation by C `+0x5C`; Y rotation by C `+0x58`; X rotation by (owner `+0xC0` + C `+0x54`).

Hand matrix C `+0x12C` (RVAs `0x020A4E–0x020BBC`): copy of the creature matrix; translate (0, 0, b); translate (C `+0x6C`, `+0x70`, `+0x74`); X rotation by owner `+0x58` × 2, skipped when (`+0x124` bit 0 and `+0x5C` bit 2) or the selector is 2, 3 or `0x25`; X rotation by 60 when `+0x124` bit `0x400`; translate (0, 0, −4); Y rotation by C `+0xA0`; translate (0, 0, 4); Z rotation by C `+0x5C`; Y rotation by C `+0x58`; X rotation by C `+0x54`.

## 8. Main-hand weapon matrix and point A

Only when C `+0xDC` is nonnull (RVAs `0x021438–0x021B1E`). The matrix C `+0x2AC` first receives a copy of the hand matrix. The carried branch is taken when (owner byte `+0x68` is one of the 15 values 0, `0x15`, `0x6D`, `0x6A`, `0x6B`, `0x69`, `0x5B`, `0x60`, `0x50`, `0x52`, `0x55`, `0x4F`, `0x53`, `0x51`, `0x54`, or `+0x124` bit `0x10` is set) and `+0xAA9` is neither 4 nor `0x0C`. Otherwise the held branch is taken. In this section "kind" means owner byte `+0xAA9`. An ordinary creature with `+0x68` equal to zero therefore uses the carried branch unless its main-hand kind is 4 or `0x0C`.

Carried branch: replace the matrix with a copy of the body matrix; Z rotation by the narrowing of (widened ((q × 0.25) × 180) ÷ binary64 pi, bits `0x400921FB54442D18`, image `0x2FCE08`); translate (0, −7.1, 0.1) (bits `0xC0E33333`, `0x3DCCCCCD`); axis-angle −45 (kind `0x0A`) or −135 (otherwise) about (0, 1, 0); axis-angle 90 about (0, 0, 1); uniform scale by the main-hand scale local; translate (sx × −0.5, sy × −0.5, sz × −0.5).

Held branch: translate by the main-hand position local; Z, Y, X rotations by its angle local components 2, 1, 0; axis-angle by the zero turn component 2 about (0, 0, 1), by component 1 about (0, 1, 0), and by (owner `+0x58` × 0.5) + component 0 about (1, 0, 0); uniform scale by the main-hand scale local; then one grip translation chosen in this order:
- model pointer equal to entry 840 of the supplied model collection (RVA `0x0120C0`, null when absent): ((sx × −0.5) − 1, 0.5, 1);
- kind `0x0A`: (sx × −0.5, sy × −0.5, −10);
- kind `0x0C`: uniform scale 0.6 (bits `0x3F19999A`) first, then (sx × −0.5, binary32 of the integer 1 − sy, sz × −0.5);
- kind 6: (sx × −0.5, 1.5 − sy, sz × −0.5);
- kind 7: (sx × −0.5, 0, sz × −0.5);
- kind 8: (sx × −0.5, −2, −4);
- kind 4: (sx × −0.5, sy × −0.5, sz × −0.5);
- otherwise: (sx × −0.5, sy × −0.5, −4).

After drawing, S1's first point = point transform of (sx × 0.5, sy × 0.5, 0) and then A = point transform of (sx × 0.5, sy × 0.5, sz), each through C `+0x2AC` and copied by three word moves. All half factors use bits `0x3F000000` or `0xBF000000`.

## 9. Off-hand weapon matrix and point B

Only when C `+0xE0` is nonnull (RVAs `0x021B23–0x022438`). The matrix C `+0x2EC` first receives a copy of the hand matrix. The carried branch requires `+0x68` in the same 15-value set (without the `+0x124` bit `0x10` alternative) and `+0x991` not `0x14`, 4 or `0x0C`. In this section "kind" means owner byte `+0x991` unless stated.

Carried branch: copy of the body matrix; translate (0.5, −0.65, 0.5) (bits `0xBF266666`); the same swing Z rotation as Section 8; then for kind `0x0D` translate (0, −8.2, 0) and axis-angle 180 about (0, 0, 1); for kind 6 or 7 translate (0, −6.2, 0), axis-angle −45 about (0, 1, 0) and 90 about (0, 0, 1); otherwise translate (0, −7.2, 0), axis-angle −225 about (0, 1, 0) and 90 about (0, 0, 1); then uniform scale by the off-hand scale local and translate (sx × −0.5, sy × −0.5, sz × −0.5).

Held branch: translate by 0.5 × (main-hand position + off-hand position) + (0, 0, 8) when the selector is `0x47`, otherwise by the off-hand position; Z, Y, X rotations by the off-hand angle local; the three zero-turn axis-angle rotations as in Section 8 using the off-hand turn local; uniform scale; then one grip translation in this order: entry 840 match: (1 − (sx × 0.5), 0.5, 1); selector `0x47`: (sx × −0.5, sy × −0.5, sz × −0.5); kind 6: (sx × −0.5, 1.5 − sy, sz × −0.5); kind 7: (sx × −0.5, 0, sz × −0.5); kind `0x0C`: scale 0.6 then (sx × −0.5, binary32 of 1 − sy, sz × −0.5); kind `0x0D`: (sx × −0.5, 3, sz × −0.5); main-hand kind `+0xAA9` equal to 8: (sx × −0.5, −2, −4); kind 4: Y rotation by 180 (RVA `0x0244F0`) then (sx × −0.5, sy × −0.5, sz × −0.5); otherwise (sx × −0.5, sy × −0.5, −4).

Both branches end with S3's first point = transform of (sx × 0.5, sy × 0.5, 0) and B = transform of (sx × 0.5, sy × 0.5, sz) through C `+0x2EC` (RVAs `0x02234D–0x022438`).

## 10. Unconditional shift and later series branches

After both blocks, whether or not a model was present, the routine builds the shift d = (fixed-to-float of ((−P.X) − C `+0x880`), fixed-to-float of ((−P.Y) − C `+0x888`), 0) and adds it, d as first operand, to the first points of S3, S2 (B), S1 and S0 (A), in that order (RVAs `0x0225FD–0x0226F1`). Consequently a refresh with a null weapon model, including mask bit `0x40` and the arrow exclusion, leaves the previous value plus the new d in A or B: the stale value accumulates one shift per refresh.

Then exactly one branch runs (RVAs `0x0226F6–0x022FE1`). T1 = RVA `0x03CAA0`, T2 = RVA `0x047310`, D = RVA `0x03D1A0`, H = RVA `0x044270`, each evaluated on the owner with its own ability byte; all comparisons are signed 32-bit.
- Selector `0x45`, clock C `+0x4` > T1 and < T1 + T2: a local matrix is reset, translated exactly as in Section 6 step 2, and Z-rotated by owner `+0x137C`. Outer radius ro = ((binary32 of (clock − T1)) × 20) ÷ (binary32 of (T1 + T2)) and ri = ro × 0.5. For i = 0 to 15, theta = narrowing of (((binary64 i) × pi) × 0.0625 − pi/2) in binary64; sine and cosine are the imported results for widened theta, each narrowed; S1[i] = (transform of ri × (sine, cosine, 0)) + d and S0[i] = (transform of ro × (sine, cosine, 0)) + d. A is replaced (it includes d); B is not touched.
- Selector in the 15-value set {`0x1E`, `0x1F`, `0x20`, `0x0E`, `0x0C`, `0x0A`, 8, `0x1C`, 1, `0x1D`, 2, 3, `0x30`, `0x31`, `0x23`}, clock > T1 and clock < D: share k = zero approached to one at rate 0.05 (bits `0x3D4CCCCD`) n times; for i = 1 to 15 each of S0, S1, S2, S3 point i += k × (point i−1 − point i). A and B are unchanged.
- Selector `0x0F`, `0x11` or `0x13` and owner `+0x6C` > T1: k at rate 0.05; B becomes (0 × (B − S3[0])) + B, which preserves finite nonzero values, can turn a negative zero into positive zero, and makes B NaN when the difference is infinite or NaN; for i = 1 to 15, S1[i] and S0[i] copy S1[0], and S3[i], S2[i] lag as above. A is unchanged.
- Selector `0x10`, `0x12`, `0x14` or `0x32`, `+0x6C` > T1 and `+0x6C` < ((H halved toward zero) + T1) + T2: k at rate 0.075 (bits `0x3D99999A`). For `0x32`, A = creature-matrix transform of (20, 0, −12) and S1[0] = transform of (8, 0, −12), with no d added, so A is then in the unshifted creature frame. Otherwise S1[0] = (0.5 × S1[0]) + (0.5 × A) and A becomes (0 × (A − S1[0])) + A with the same zero and NaN effects. For i = 1 to 15, S0 and S1 lag and S3[i], S2[i] copy S3[0]. B is unchanged.
- Otherwise: for i = 1 to 15, S0[i] = A, S1[i] = S1[0], S2[i] = B, S3[i] = S3[0].

## 11. Values before the first refresh

The subobject constructor RVA `0x011E50` (called from creature constructor RVA `0x03B690` at RVA `0x03BB4D`) stores zero to C words `+0x4` through `+0xBC`, `+0xC4` through `+0xDC`, `+0xE4`, `+0xE8`, `+0x86C` through `+0x874`, the byte `+0x878` and the references `+0x880` through `+0x88C`. It stores nothing in `+0xEC` through `+0x86B`, so no matrix and no series point, A and B included, is initialized; `+0xC0` and `+0xE0` are also left. The creature constructor's only block clear is 256 bytes at creature `+0x1D5C` (RVA `0x03BBDC`). All eight call sites of the creature constructor (RVAs `0x062019`, `0x064FE1`, `0x070304`, `0x08238F`, `0x087612`, `0x20E814`, `0x20FFDB`, `0x21592D`) obtain the object from the imported allocator for `0x1E60` bytes with no clearing between allocation and construction. Therefore A and B hold whatever bytes the allocator returned until a refresh with a nonnull model writes them, and a refresh with a null model adds d to those bytes. No whole-creature copy was found: an image byte scan of executable sections finds `0x00001A04` and `0x00001B84` only at the four renderer creature-field sites of Section 12, at stack-local addressing (RVAs `0x0B49D8`, `0x09286C`) and at two call displacements, `0x1E60` appears only in allocations, and every byte-encoded direct call to RVA `0x0128F0` is one of the six sites listed below.

## 12. Frame order and staleness

The frame routine RVA `0x0C85F0` reads the imported millisecond timer, forms elapsed = now − previous (previous taken equal to now on the first frame, so the first elapsed is zero), calls the application's slot `0x28`, then its slot `0x2C`, then presents. For the client class whose table begins at image `0x2FFDC8`, slot `0x28` is the update RVA `0x088EE0` and slot `0x2C` is the renderer RVA `0x0AC260`. The update calls the world routine RVA `0x20C510` (at RVA `0x08CF2E` in a loop with count 20 and at `0x08CF49` with the remainder); that routine's refresh of P and owner `+0x1374–0x137C` is outside this contract. The update also stores the pose repeat count at client `+0x8006E8` (RVA `0x08CE42`). All world-routine work of a frame therefore precedes that frame's renderer.

Inside the renderer: the light loop (`0x0AF602–0x0B06E9`) reads A at RVA `0x0B0200` and B at RVA `0x0B037D`. It runs before the draw loop (`0x0B39E0–0x0B76BE`), and no refresh call precedes it in the renderer. In the draw loop each admitted creature first gets C `+0x4` = owner `+0x6C` and a selector chosen from owner byte `+0x68` by tables at images `0x0BBCDC` and `0x0BBBCC` (default 0), overridden to `0x38` when owner signed `+0x130` > 0 (RVAs `0x0B4EE5–0x0B5338`). Then, when an after-image gate holds, five refreshes run at RVA `0x0B5CF3` with n = 0. Then, if owner `+0x1190` is not greater than zero, the main refresh runs at RVA `0x0B5DB2` with n = client `+0x8006E8` and references client `+0x1D8`/`+0x1E0`. Otherwise the creature is appended to a deferred list (RVA `0x0860B0`) and refreshed later at RVA `0x0BA8FE` with the same n, then again at `0x0BA9A5` with n = 0. The after-image and deferred calls pass renderer locals observed to hold the same two client reference addresses. The series geometry loops reading from A and B at RVAs `0x0B9DF0` and `0x0B9F82` follow the draw loop.

Thus, for an ordinary admitted creature, the light reads of a frame see A and B from the last refresh of an earlier renderer call (for a deferred creature, the later n = 0 call): one frame stale relative to that frame's world update and references, or allocator bytes if the creature has never been refreshed. The later geometry reads see the current frame's main or deferred refresh. A creature not admitted to the draw loop keeps its old A and B. Two other routines also call the refresh (RVA `0x0269E1` in RVA `0x025450` and RVA `0x207E31` in RVA `0x205AE0`, reached through tables at images `0x2FCEE0` and `0x31E0A0`); their scheduling is not closed.

## 13. Random draws

The transitive direct-call closure of RVA `0x0128F0` (80 routines including itself, from `decompiled/client/calls.csv`), together with the closures of every direct callee of the case bodies, reaches neither the imported `rand` nor `srand`. Its unresolved indirect calls are graphics-device and shader-object slots. No game-owned generator was identified in the chain. This is not a proof about those device callbacks.

## 14. Limits and open items

Open: the per-selector evaluator arithmetic (Section 4), and the internals of the owner timing helpers, the model lookup, entry 840's identity and the after-image gate. Also open: draw-loop admission and the derivation of client `+0x8006E8`, and the scheduling of the two other refresh callers. Ambient x87 and MXCSR state, denormals, imported math accuracy, non-finite or out-of-range float-to-integer results, 64-bit overflow, invalid pointers, locking and concurrent writers are limits outside this contract.
