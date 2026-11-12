Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha client: head-pitch sites in the pose refresh, creature-pick visibility and box, and the target-id consumers

## Scope and evidence

This contract closes, as far as the original image supports, the items that the held contract `alpha-controller-aim-writers.md` (sections 3.2, 7 and 8) left open: the head-pitch read sites in the pose refresh `0x0128F0`, the maximum-distance argument and target point of the creature pick's visibility call, the consumers of the creature target-id field written for action `0x22`, and the corner layout and homogeneous handling of the pick's eight-corner box. The held contracts `alpha-controller-aim-writers.md` and `alpha-weapon-attachment-poses.md` were used only to locate routines and for their terms; every statement below was re-checked in the original.

Supported image: client `binary/Cube.exe`, 32-bit, preferred base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (recomputed before use). `binary/Server.exe` (SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, recomputed) was not otherwise used. All addresses are RVAs. Evidence: `decompiled/client/assembly-functions/004128f0.asm` (pose refresh), `00488ee0.asm` (controller update), `004ac260.asm` (renderer), `decompiled/client/assembly.txt` (helpers, case bodies that the per-function files omit, the world update `0x20C510`), `decompiled/client/symbols.csv` (import names only), and constants, jump tables and static data read from the image bytes. The Infinite-Cubes alpha reconstruction was consulted only for naming leads; such leads are marked "(reconstruction lead)" and are not presented as facts.

Terms: "owner" or "K" is a creature; C is a creature's pose subobject at creature `+0x1498` (as in the weapon contract, C offset + `0x1498` = creature offset); the client object, P0, E (camera eye, client `+0x140`), N (elapsed milliseconds) and the flags word (creature `+0x124`) are as in the held controller contract. Fixed values are signed 64-bit with 65,536 units per world unit. "Fixed-to-float" is the x87 load of the 64-bit integer, a store to binary32 under the ambient x87 rounding control, then a binary32 multiply by 2^-16. "Float-to-fixed" is binary32 × 65,536 followed by the runtime 64-bit conversion `0x28D946`. All binary32 arithmetic rounds after each operation; there are no fused operations. Stack locals of a routine are identified by their offset from that routine's steady stack pointer (pending pushes accounted for).

## 1. Head pitch in the pose refresh `0x0128F0`

### 1.1 Matrix and rotation conventions

Matrices are 4-by-4 binary32 with element index row + 4 × column. Every matrix helper used below right-multiplies the current matrix M (M becomes M·T), so in each chain the step listed last acts first on model-space points, and each rotation turns the part about its own current local axis.

- **Axis rotation about local X, `0x0243D0`.** The degree argument is multiplied in binary32 by `0x3C8EFA35` (π/180, image `0x2FCDA4`); the rounded radian value is widened to binary64; c is the narrowed result of the imported precise cosine and s of the precise sine (MSVCR110 `libm_sse2_cos_precise`, then `libm_sse2_sin_precise`). For each row r, with the old values, new column 1 = (column 2 × s) + (column 1 × c) and new column 2 = (column 2 × c) − (column 1 × s). This is right-multiplication by a rotation whose column 1 is (0, c, s) and column 2 is (0, −s, c): a positive angle turns the part's local +Y axis toward its local +Z axis. The sibling helpers `0x0244F0` (local Y) and `0x024610` (local Z) have the forms stated in the weapon contract, section 2.
- **Axis-angle rotation, `0x0241B0`** (angle a, axis x, y, z): as in the weapon contract, section 2, re-checked here. The axis length L is the narrowed binary64 square root of ((x·x + y·y) + z·z); the matrix is unchanged only when L compares ordered-equal to zero (an unordered L proceeds); otherwise a full rotation matrix R is formed from the normalized axis with t = 1 − c and M is replaced by M·R through `0x012400`. Element 6 (row 2, column 1) is t·(y'·z') + s·x' and element 9 (row 1, column 2) is t·(y'·z') − s·x'. For the axis (1, 0, 0) this gives the same rotation sense as `0x0243D0`. It is not bit-identical to `0x0243D0`: its first diagonal element is (1 − c) + c rounded, and it performs a full four-term product per element.
- Translate `0x024A60` and scale `0x024730` are as in the weapon contract, section 2.

### 1.2 Part identification

On entry the refresh fills C's model-pointer slots from 16-bit model indices in the owner (`0x01296E–0x012BC0`). C `+0xBC` and C `+0xC0` are first zeroed and then always looked up: C `+0xBC` from the signed 16-bit index at owner `+0x8C`, C `+0xC0` from owner `+0x8E`. Each lookup yields null when the index is negative or not below the model collection's element count (collection `+0x8` minus `+0x4`, divided by 4). The other slots first try an equipment record and fall back to an index: C `+0xCC` (record owner `+0x760`, index `+0x90`), C `+0xD8` (`+0x648`, `+0x92`), C `+0xC4` (`+0x530`, `+0x94`), C `+0xD4` (index `+0x96`), C `+0xD0` (index `+0x9A`), C `+0xC8` (index `+0x98`).

The reconstruction names the indices `+0x8C`…`+0x9A` head, hair, hands, feet, chest, tail, shoulder and wings, and the matrices C `+0xEC`, `+0x12C`, `+0x1AC`, `+0x1EC` body, hands, head and hair (reconstruction lead). The original corroborates head and hair structurally only. The C `+0x1AC` chain is scaled by owner `+0x9C`, the first scale float of the appearance block, and is placed by the owner triplet `+0xE8`, `+0xEC`, `+0xF0`. It also carries the head-pitch rotation and the creature-rotation correction (1.3). C `+0x1EC` repeats the same chain with the second model and colours its draw from the owner's three colour bytes at `+0x7A`. No string or table in the image names the parts. Below, C `+0x1AC` is called the head part, C `+0x1EC` the hair part and C `+0x12C` the arm (hands) matrix, as working labels.

Pose angle triplets used below: C `+0x54`, `+0x58`, `+0x5C` (blended body angles), C `+0x6C` triplet (blended body offset), C `+0x78` triplet (blended head offset), C `+0x84`, `+0x88`, `+0x8C` (blended head angles; each component approaches its target through the angle-approach helper `0x023F70` at `0x01EC8E–0x01ED89`), C `+0x9C` triplet (blended hand turn). The bob value b and the creature matrix are as in the weapon contract, sections 6 and 7.

### 1.3 Sites `0x02059D` (head) and `0x020831` (hair)

Run only when C `+0xBC` (head) respectively C `+0xC0` (hair) is non-null (`0x020433`, `0x0206C7`). The chain for the head part (`0x020440–0x0206A5`), in order of the calls:

1. Copy the body matrix C `+0xEC` into C `+0x1AC`. The body matrix is the creature matrix followed by translate (0, 0, b), translate (C `+0x6C`, `+0x70`, `+0x74`), Z rotation by C `+0x5C`, Y rotation by C `+0x58`, and X rotation by (owner `+0xC0` + C `+0x54`) (`0x01F611–0x01F71B`).
2. Translate by (C `+0x78` + owner `+0xE8`, C `+0x7C` + owner `+0xEC`, (C `+0x80` + owner `+0xF0`) − (b × 0.5)).
3. Translate (0, 0, −3.0) (bits `0xC0400000`).
4. X rotation by ((−C `+0x54`) − owner `+0xC0`); the negation is an exact sign toggle.
5. Y rotation by −C `+0x58` (exact sign toggle).
6. Z rotation by (C `+0x8C` − C `+0x5C`).
7. Y rotation by C `+0x88`.
8. **X rotation by ((owner `+0x58` − owner `+0x1374`) + C `+0x84`)** at `0x0205BA`: the head pitch, minus the first component of the owner's drawn-rotation triplet, plus the blended head-angle X component (the held contract's "pose value").
9. Translate (0, 0, 3.0) (bits `0x40400000`).
10. Uniform scale by owner `+0x9C`.
11. Translate (sx × −0.5, sy × k, sz × −0.5), where sx, sy, sz are the model's signed 32-bit dimensions (getters `0x024810`, `0x024820`, `0x07B5F0`) converted to binary32; k is −0.2 (bits `0xBE4CCCCD`, image `0x2FCE84`) when the owner's signed 32-bit `+0x64` equals `0x65`, and −0.5 otherwise.
12. Draw (`0x0482A0`).

Steps 4 to 6 invert the body matrix's rotations in reverse order (X, then Y, then Z less the head's own Z). Up to rounding, the head's orientation is therefore the creature matrix's rotation followed by Z by C `+0x8C`, Y by C `+0x88` and the step-8 X rotation. The body offsets and the head pivot translations still apply. No head step doubles or halves the head pitch.

The hair chain (`0x0206D4–0x020A2D`) is identical with C `+0x1EC` and the hair model, except step 11: the y factor is always −0.5. The hair is then drawn with a colour built from the owner bytes at `+0x7A` (first three components, each converted to binary32 and divided by 255.0, fourth 1.0). The colour is used as is when bit `0x400` of the owner's 16-bit `+0x7E` is set, and multiplied by the refresh's colour argument otherwise (`0x020902–0x020A17`).

**Owner `+0x1374`.** Owner `+0x1374`, `+0x1378` and `+0x137C` form the drawn-rotation triplet. The creature matrix applies Z by `+0x137C`, Y by `+0x1378` and X by `+0x1374` (weapon contract, section 6). The world update `0x20C510` sets this triplet from the creature's entity rotation triplet (`+0x28`, `+0x2C`, `+0x30`): a direct copy when the signed 32-bit `+0x1398` is not positive (`0x21FD36–0x21FD4F`, together with copying `+0x10` into `+0x1350`). While `+0x1398` is positive, each component approaches its target through the angle-approach helper `0x023F70` (`0x21FB7A–0x21FD1F`). Copies from another creature occur at `0x21FA5E` and `0x220FED`. The creature constructor zeroes the triplet (`0x03B872–0x03B886`). Step 8 subtracts the drawn X angle that the creature matrix applies (its last rotation before the creature-angle and lift terms). The two cancel exactly only when the intervening creature-matrix terms (the C `+0xAC`/`+0xB0` translation, the C `+0x68`, `+0x64`, `+0x60` rotations, the lift translation and the scale) leave the X axis unchanged.

### 1.4 Site `0x020AC6–0x020B1D` (arm matrix C `+0x12C`)

The arm matrix chain (`0x020A4E–0x020BBC`) is as in the weapon contract, section 7, re-checked here: copy of the creature matrix; translate (0, 0, b); translate (C `+0x6C`, `+0x70`, `+0x74`); then:

- **X rotation by owner `+0x58` × 2.0** (constant `0x40000000`, image `0x345E10`; an exact doubling) through `0x0243D0`, unless (bit `0x0001` of the owner's flags word `+0x124` is set **and** bit `0x04` of the owner's 32-bit `+0x5C` is set) or the pose selector C `+0x8` equals 2, 3 or `0x25`. No pose value is added at this site.
- Independently, when bit `0x0400` of the flags word is set, an X rotation by 60.0 (`0x42700000`).
- Translate (0, 0, −4.0); Y rotation by C `+0xA0` (the hand-turn Y component); translate (0, 0, 4.0); Z rotation by C `+0x5C`; Y rotation by C `+0x58`; X rotation by C `+0x54`.

The arm chain, unlike the body matrix, does not include owner `+0xC0`. The doubled pitch therefore pitches the arm matrix, and everything copied from it (the held weapon branches), by twice the head pitch.

**The value tested at C `+0x8`** is not a part code. It is the pose selector, which the renderer writes before the main refresh from the owner's action byte `+0x68` through the byte table at image `0x0BBCDC` and the jump table at `0x0BBBCC` (`0x0B4EE5–0x0B5338`); it is overridden to `0x38` when the owner's signed `+0x130` is positive. In that assignment selector 2 comes from actions `0x39` and `0x3C`, selector 3 from action `0x3A`, and selector `0x25` from action `0x43` (case bodies at `0x0B4F9F`, `0x0B4FCC`, `0x0B509E`, read from `assembly.txt`). The reconstruction names these actions two-handed attacks (reconstruction lead). Per-selector case bodies can set the selector to zero during the refresh (weapon contract, section 4); other writers of C `+0x8` were not audited.

**Gate fields.** For the local player, bit `0x0001` of the flags word is copied from input +0x13 every time the controller's flags block runs (held contract, section 4). Bit `0x04` of `+0x5C` is set by the world update's collision handling (`0x21D98C`, `0x21DADC`, `0x21DC56`, `0x21DDA6`, `0x21E0B4`, `0x21E2EF`, `0x21E52F`, `0x21E843`). The same combination (`+0x124` bit 0 with `+0x5C` bit 2) raises both hands in the post-evaluator corrections (weapon contract, section 5). The reconstruction calls these bits climbing and touching a wall (reconstruction lead); the original establishes only the producers named here. Bit `0x0400`'s producer was not traced here; the same bit selects the (−70, 0, 0) creature-angle pose target (weapon contract, section 3).

### 1.5 Sites `0x0217AA` (main hand) and `0x021FE3` (off hand)

These sites lie in the held branches of the two weapon matrices (weapon contract, sections 8 and 9), reached only when the respective weapon model pointer (C `+0xDC`, C `+0xE0`) is non-null and the held branch is selected. Order within each branch: translate by the hand-position local; Z, Y, X rotations (`0x024610`, `0x0244F0`, `0x0243D0`) by that hand's angle-local components 2, 1, 0. Then three axis-angle rotations through `0x0241B0`: by the hand's turn-local component 2 about (0, 0, 1), by its component 1 about (0, 1, 0), and **by (owner `+0x58` × 0.5) + turn-local component 0 about (1, 0, 0)**. The half constant is `0x3F000000` (image `0x345DA0`). Then follow a uniform scale by that hand's weapon-scale local (the triple-equal `0x024730` call), the grip translation and the draw.

The per-hand turn locals (main hand at the refresh's steady `+0x248`, off hand at `+0x254`) are zeroed at `0x012CD3–0x012D0A` (weapon contract, section 3, states that nothing writes them afterwards; not independently re-audited). The "pose value" at these sites is therefore zero, and the weapon is pitched by half the head pitch on top of the arm matrix's doubled pitch. This adds up to 2.5 × the head pitch only when the rotations between the two sites (arm Y, Z, Y, X angles, the hand angle locals and the zero turns) are zero and the arm gate admits the doubling. The rotation sense is that of `0x0243D0`, up to the rounding difference noted in 1.1.

### 1.6 Draws

None of the chains above calls `rand`. The routine's draw-free status is as in the weapon contract, section 13.

## 2. Creature-pick visibility and point (controller `0x088EE0`)

### 2.1 Light-scaled maximum distance

**Day factor D, computed once per update before the creature scan** (`0x09858F–0x098637`). Let t be the world's signed 32-bit millisecond clock at world `+0x80015C` (read by `0x071910`). Then v = (((binary32 of t) × 2.0) ÷ 86,400,000.0) − 1.0 (constants `0x40000000`, `0x4CA4CB80` at image `0x302AC4`, `0x3F800000`). Then p = pow(v, 4.0) and D = pow(1.0 − p, 5.0). Each power widens both binary32 operands, calls the imported `libm_sse2_pow_precise` and narrows to binary32 (`0x011D10`), and 1.0 − p is binary32. D is 1 at t = 43,200,000 and approaches 0 near t = 0 and t = 86,400,000.

The world update advances the clock by N × 10, or by N × 100 when a byte flag is set (`0x20C67E–0x20C68C`). It forces the clock to 32,400,000 while world `+0xA4` is zero, and while the clock exceeds 86,400,000 it subtracts 86,400,000 and increments world `+0x800160` (`0x20C6A5–0x20C6E2`). The clock is initialised to 43,200,000 at `0x18EDE5`; another store lies at `0x1A57DB`. The world clock is thus a time of day in milliseconds, and D a daylight factor. The flag byte and the store at `0x1A57DB` were not traced.

**Per creature K** (`0x0986E1–0x098777`):

1. m = 256.0 (`0x43800000`, image `0x302AB8`).
2. A block light value Lt is read by `0x0718B0` on the world at the block coordinates of K's entity position (K `+0x10`, `+0x18`, `+0x20`). Each 64-bit component is divided by 65,536 with the runtime signed division (`__alldiv`, truncation toward zero; `0x02F570`) and the low 32 bits are passed. `0x0718B0` locates the block through `0x02F7E0` (column lookup `0x0347A0` by x and y, then the column's vertical range). Then:
   - The low five bits of the block's byte 3 equal to `0x0D` give 255.
   - A type value (those five bits) other than 0 or 2 gives 0.
   - Otherwise the result is the block's byte 0, raised to 5 when below 5.

   Missing columns and positions below a column give the static block at VA `0x0076AFC4` (bytes C8 C8 C8 01, giving 0). Positions above a column, or empty blocks at z ≤ 0 without bit `0x40` of byte 3, give the static blocks at VA `0x0076AFC0` (FF FF FF 00) or `0x0076AFBC` (FF FF FF 82). Both give 255. The three static blocks are set by the initializers `0x2F9720`, `0x2F9750`, `0x2F9780`.
3. s = ((binary32 of Lt) ÷ 255.0) × D.
4. If 0.3 > s (ordered; constant `0x3E99999A`), m = (((s × 0.9) ÷ 0.3) × 256.0) + 25.6 (constants `0x3F666666`, `0x3E99999A`, `0x43800000`, `0x41CCCCCD`). Otherwise m stays 256.0.

So the pick reaches 256 units for a creature standing in light at full daylight (s ≥ 0.3), and only down to 25.6 units for a creature in total darkness. The light is sampled at K's entity position `+0x10`, not at the pick point below.

### 2.2 Pick point and the field `+0x1180`

The visibility target point is Q_K = K `+0x1350` + float-to-fixed of (0, 0, −K `+0x1180`), using 64-bit component adds (`0x02C800`; `0x09877D–0x0987E9`). The same lowered point is the centre of the eight-corner box (section 4). The two squared distances of the pick, from E and from P0's entity position `+0x10`, use K `+0x1350` without this lowering (`0x098B37–0x098B66`, `0x098BF0–0x098C25`).

**Meaning of `+0x1180`** (binary32, world units). It is the amount by which a creature's physical position stands above the height at which it is drawn and picked. The creature matrix translates by (P.Z − float-to-fixed(`+0x1180`)) as well (weapon contract, section 6). Producers, all in the world update `0x20C510` unless noted:

- **Snap absorption.** At `0x21CBD4–0x21CC11` and `0x21D4E0–0x21D583` (the latter inside a search over block heights that, on finding a block accepted by `0x03B480`, sets bit `0x01` of `+0x5C`), the creature's vertical position is replaced by a snapped value. `+0x1180` is then set to the fixed-to-float of (new z − (old z − float-to-fixed(old `+0x1180`))), that is, the snap distance plus the previous value. The drawn height (z − `+0x1180`) is therefore continuous across the snap. The exact snap targets are not closed here.
- **Decay** (`0x21F081–0x21F0C3`). Each world step, `+0x1180` approaches zero through the repeated-approach helper `0x011D40` (weapon contract, section 2), with the step's repeat count and rate 0.01 (`0x3C23D70A`) when the value is negative (0.0 > value, ordered) and 0.02 (`0x3CA3D70A`) otherwise.
- **Zeroing.** At `0x21FD2A` (while the creature's `+0x1398` is positive, see 1.3), in the creature constructor (`0x03B6FF`) and at `0x20B9DE` (routine `0x20AB30`).
- **Copies** from another creature at `0x21FAFF` and `0x221088` (with `+0x1188` and `+0x118C`).

Within one controller update, the pick reads `+0x1180` as left by that update's world step (held contract, section 1.2).

### 2.3 The visibility routine `0x19EE90`

Called at `0x0987FE` on the world with arguments (E, Q_K, 0, m). For eye A, target B, option c and maximum m:

1. d = B − A per component in 64-bit; dx, dy, dz are the fixed-to-float values of d.
2. q = ((dy × dy) + (dx × dx)) + (dz × dz).
3. If 0.0001 > q (ordered; `0x38D1B717`), the result is visible.
4. Otherwise, if q > m × m (ordered), the result is not visible.
5. Otherwise len = narrowed binary64 square root (imported `libm_sse2_sqrt_precise`) of widened q, and u = (dx ÷ len, dy ÷ len, dz ÷ len). The world ray `0x1A35D0` is called from A in direction u with maximum length len and option arguments 0 and then c (here 0). Its x87 result is stored as binary32 r. The result is visible if and only if r ≥ len − 0.01 (`0x3C23D70A`); an unordered comparison gives not visible.

K is skipped by the pick when the routine returns not visible. What the ray treats as a hit and returns on no hit was not audited (held contract, section 8).

## 3. Consumers of the target-id field (creature `+0x1A0`/`+0x1A4`)

The field is a signed 64-bit creature id at creature `+0x1A0` (low word) and `+0x1A4` (high word), which is entity-record `+0x190` (record at creature `+0x10`). The controller writes it for P0 at `0x098E4E–0x098E76` (held contract, section 3.2). Located consumers:

### 3.1 World update, action `0x22` case (`0x214C32–0x214F77`)

The world update's per-action switch (`0x211E01–0x211E1B`; index byte table at image `0x222CCC`, jump table at `0x222C84`) sends only action `0x22` to entry 5 (`0x214C32`). Let T1 = `0x03CAA0`(−1) and T2 = `0x047310`(−1) on the creature, clock = signed `+0x6C`, and N the step's elapsed milliseconds. The case's own code reads as follows; its case body is not attributed to the routine in the per-function listing.

1. **Wind-up sound** (all worlds). If clock < T1 and the signed quotient of (clock + N) by 100 differs from that of clock by 100, one `rand` value is drawn. A sound record is queued with the creature's entity position (`+0x10`, fixed-to-float), sound id `0x24` and pitch ((binary32 of rand × 0.25) ÷ 32,767.0) + 1.0 (`0x214C32–0x214CE6`).
2. **Gate.** The routine continues only if the world byte `+0xB4` is zero and the creature's byte `+0x60` is nonzero, or if the creature is the world's local creature at world `+0xB8` (`0x214CEB–0x214D06`). The reconstruction reads `+0xB4` as "client world" and `+0x60` zero as a player (reconstruction lead).
3. **Timing.** Continue only if T1 < clock ≤ T1 + T2. Let period = the truncating conversion of ((binary32 of T2) × 0.125). Continue only if the signed quotients (clock + N − T1) ÷ period and (clock − T1) ÷ period differ (`0x214D0C–0x214DA9`). A period of zero is not guarded.
4. **Mana.** cost = `0x044AE0`(action, −1). Continue only if the binary32 at `+0x170` is not less than cost (an unordered comparison stops). Then `+0x170` = `+0x170` − cost, recomputing cost (`0x214DAF–0x214E1C`).
5. **Target.** The world's creature-by-id lookup `0x02F000` is called with the address of the creature's `+0x1A0` (`0x214E16–0x214E36`). When it returns null, the caster itself is the target.
6. **Heal record.** A hit record is built (`0x059A30`) with:
   - the target's entity position `+0x10`;
   - one `rand` value giving roll = 1.25 − ((binary32 of rand × 0.5) ÷ 32,767.0);
   - amount = (spell power `0x046E80` × −2.0) × roll;
   - the critical byte from `0x0444A0`, which draws one further `rand` (it compares a chance with rand ÷ 32,767.0);
   - attacker id = the caster's id (`+0x8`/`+0xC`) and target id = the target's id.

   The record is queued on the outgoing hit list (`0x086290`). A sound record with the caster's position, pitch 1.0 and sound id `0x29` is then queued (`0x0864C0`) (`0x214E39–0x214F2C`).
7. **Apply.** If world `+0xB4` is zero, or both the caster and the target are world `+0xB8`, the hit is applied at once through `0x195A60` (`0x214F31–0x214F72`). Otherwise it is only queued.

Hence on its own client, the local player's healing stream heals the id the controller stored (held contract, section 3.2: the third id pair, or P0 itself under input +0x14). The heal is applied locally only when that target is the local player. A missing or unknown id falls back to the caster.

### 3.2 Renderer (`0x0B454E–0x0B4676`)

For each creature in the draw loop, the renderer proceeds only if the target id is nonzero, the action is `0x22`, and the clock is below `0x03D1A0`'s result (R0 of the held contract). It then looks up the target by id (`0x02F000`). If the target is found and the client test `0x07F760` succeeds, it calls the effect routine `0x0BBD80`. The test receives the target's `+0x1350` position, the target's `+0x88` (third size component) and a renderer local. The effect routine receives the target's `+0x1350` position, the colours (0.25, 0.75, 1.0, 1.0) and (0.0, 0.0, 1.0, 1.0), the caster's clock, two renderer locals, the target's `+0x80` (first size component), 0.1 and 20. The internals of `0x07F760` and `0x0BBD80`, and the identity of the two renderer locals, were not closed. The renderer therefore draws the healing-stream effect toward the targeted creature during the first R0 milliseconds of the action.

### 3.3 Copies and network fields

The record copy routines `0x04B040` (at `0x04B275–0x04B287`) and `0x066050` (at `0x066284–0x066296`) copy record `+0x190`/`+0x194`. The entity field reader `0x0CCFA0` (`0x0CD2C2`) and the stream receiver `0x0CD3E0` (`0x0CD8F0`) read it as one masked field, in sequence between record `+0x188` and `+0x198`. For P0 the received value is discarded on the client (held contract, section 6). The matching writer in the send path was not located. Many other code references to `+0x1A0`/`+0x1A4` exist but belong to other object types (widgets, device tables, runtime data); none of those that were checked is a creature target-id use.

## 4. The pick's eight-corner box and the projection helper

### 4.1 Corner construction (`0x09880B–0x098B31`)

Three counters, each running 0 then 1, are nested: outer c0 (steady local `+0x64`), middle c1 (`+0x60`), inner c2 (`+0x20`). The four flags (section 4.3) are cleared once per creature before the loops. For each corner:

1. Fixed (0, 0, −K `+0x1180`) is formed (float vector `0x00EA90`, converted to fixed by `0x02C460`).
2. The half-extents use hz = K `+0x88` + 1.0, hy = K `+0x84` + 1.0 and hx = K `+0x80` + 1.0, read again for every corner.
3. **`0x03B610`** is a plain constructor of a fixed triple from three 64-bit values passed by value. It is called first with (client `+0x1D8`, client `+0x1E0`, 0), forming the rebasing vector R = (reference X, reference Y, 0). These are the same references that the renderer passes to the pose refresh (weapon contract, section 12), which adds them to world positions to form render-space coordinates. No vertical reference is applied.
4. It is called again with the corner offset O = (float-to-fixed of ((c0 × hx) − (hx × 0.5)), float-to-fixed of ((c1 × hy) − (hy × 0.5)), float-to-fixed of ((c2 × hz) − (hz × 0.5))), each counter converted to binary32. For c = 0 the binary32 value is −(h × 0.5). For c = 1 it is h − (h × 0.5), which equals h × 0.5 exactly for finite h.
5. Corner = ((K `+0x1350` + fixed (0, 0, −K `+0x1180`)) + O) + R, three 64-bit vector adds (`0x02C800`) in that order.
6. The corner is converted with fixed-to-float per component (`0x02C460`) and transformed by the shaken view matrix client `+0x26C` with the point transform `0x0248A0` (division by w as in the weapon contract, section 2) into the transformed point T.

**Axis mapping:** c0 selects the x side (size component 0), c1 the y side (component 1), c2 the z side (component 2). Corners are visited with x outermost and z innermost: (−,−,−), (−,−,+), (−,+,−), (−,+,+), (+,−,−), …, (+,+,+). The box spans the creature's size plus 1.0 on every axis, centred on the lowered point Q_K.

### 4.2 Projection helper `0x088D60`

Only when T's component 2 is strictly greater than 0.0 (ordered; `0x098A41–0x098A53`) is T transformed in place by the matrix at client `+0x800A90` (`0x088D60`, `0x098A67`). With M that matrix and (x, y, z) = T:

- w = ((M3·x + M7·y) + M11·z) + M15;
- X = ((M4·y + M0·x) + M8·z) + M12;
- Y = ((M1·x + M5·y) + M9·z) + M13;
- Z = ((M2·x + M6·y) + M10·z) + M14;
- r = 1.0 ÷ w (binary32), and T becomes (r × X, r × Y, r × Z).

The perspective division by w is unconditional. There is no clipping, no test of w's sign and no guard against w = 0, which gives infinities or NaN that then fail the comparisons below. The only exclusion is the caller's test of the view-space component 2 before the call. Corners at or behind that plane do not contribute, and the box's edges are not clipped against it.

### 4.3 Flags

After the in-place transform, with T0 and T1 its components 0 and 1, the four flags are ORed:

- (0.0 > T0);
- (0.0 > T1);
- (T0 > 0.0);
- (T1 > 0.0).

All comparisons are ordered, so a component exactly zero or NaN sets no flag. K is a hover candidate only when all four flags are set after the eight corners. That is, at least one admitted corner lies on each side of the projected centre on both axes; the corners need not be the same one.

## 5. Random draws

- Pose refresh head-pitch chains (section 1): none.
- Pick (section 2): no direct `rand` call in `0x088EE0`'s span (held contract, section 3.5), in `0x19EE90`, in `0x0718B0` or in the day-factor computation. Draws inside the ray `0x1A35D0` and the admission routine `0x1988A0` were not audited.
- Target-id consumer (section 3.1): per world step, for each action-`0x22` creature, one draw for each wind-up sound (clock < T1, at each 100 ms boundary crossed). Each heal draws once for the roll and then once inside `0x0444A0` for the critical, in that order. Further draws inside `0x046E80`, `0x044AE0` and the hit application `0x195A60` were not audited; no direct `rand` call was seen in the scanned parts of `0x046E80` and its callee `0x03EA40`.
- Renderer consumer (section 3.2): none in the code shown; `0x07F760` and `0x0BBD80` were not audited.

## 6. Unresolved

- The part names head, hair and arm (hands) rest on reconstruction naming plus the structural corroboration of 1.2; no original string or table names them.
- The meanings of flags bits `0x0001` and `0x0400`, of `+0x5C` bit `0x04` beyond the producers listed, of owner `+0x64` value `0x65` (head y factor −0.2), of owner `+0xC0`, and the producer of `+0x124` bit `0x0400`.
- Writers of the pose selector C `+0x8` other than the renderer's assignment and the case bodies, including the after-image and the other refresh callers (`0x0269E1`, `0x207E31`).
- That nothing writes the per-hand turn locals (taken from the weapon contract, not re-audited).
- The ray `0x1A35D0` (hit rule, no-hit value, option arguments), the column lookup `0x0347A0`, the block-byte layout behind `0x0718B0` (type field, light byte), the flag byte that selects the ×100 clock rate, world `+0xA4`, and the clock store at `0x1A57DB`.
- The exact snap targets of the two `+0x1180` snap sites, the repeat count passed to its decay, and whether `+0x1180` travels in network records.
- Whether the shaken view matrix client `+0x26C` keeps row 3 equal to (0, 0, 0, 1) (if not, `0x0248A0` already divides by a non-unit w), and the full content of client `+0x800A90`.
- The internals of `0x07F760`, `0x0BBD80`, `0x044AE0`, `0x046E80`, `0x0444A0`'s chance term, `0x195A60` and `0x059A30`, and the roles of the two renderer locals passed to `0x0BBD80`.
- The send-path writer of the target-id field, writers of the field for creatures other than P0 (server or AI), and the behaviour when period = 0 in 3.1 step 3.
- Non-finite inputs, x87 and MXCSR state, imported math accuracy and 64-bit overflow are limits outside this contract.
