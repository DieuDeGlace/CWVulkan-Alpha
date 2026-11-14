Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha world objects: course steering, the angle-interpolation helper and yaw follow, creature placement on objects, the companion spawn record, and the unstuck routine's object part

## Scope and evidence

This contract closes items that the held contract `alpha-world-objects.md` left unaudited: its Section 6.1 (course steering), the angle helper of its Section 6.2 item 4, and the two consumers of its Section 6.4. That contract was read only to locate the routines and for its terms. Every statement below was re-derived from the original images; where a term of that contract is reused, its definition was re-checked against the instructions named here.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were recomputed before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs (virtual address minus `0x00400000`); client RVAs unless marked "server". Evidence: the instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, the call lists `decompiled/<image>/calls.csv`, the symbol table `decompiled/client/symbols.csv` (runtime routine names only), and the image bytes for every constant (read through the section table of each image). Decompiler pseudocode was not relied on. The isolated Infinite-Cubes alpha reconstruction was consulted for leads only; the single statement resting on it is marked "(reconstruction lead)".

### Terms

- **O**: one object of the world's object list L (world `+0x0C`), with the field names of the held contract: target position `+0x10` (3 × signed 64-bit, 65536 units per world unit), velocity `+0x28` (3 × binary32, world units per second), target yaw `+0x34` (binary32 degrees), home `+0x38` (3 × signed 64-bit), heading `+0x50` (binary32 degrees), destination `+0x58` (3 × signed 64-bit), state byte `+0x70`, current position `+0x80` (3 × signed 64-bit), current yaw `+0x98` (binary32 degrees).
- **step**: the world update's first argument, a signed 32-bit millisecond count (stored at `0x20C54F` and read unchanged by every routine below).
- **F(v)**, per component: the signed 64-bit value loaded by the x87 unit, stored to binary32 under the ambient x87 rounding control, then multiplied in binary32 by 2⁻¹⁶ (binary32 `0x37800000`; helper `0x02C4A0`, server `0x002550`).
- **T(x)**, per component: binary32 x times 65536.0 (binary32 `0x47800000`) in binary32, then converted to signed 64-bit by the runtime conversion `0x28D946`, which truncates toward zero (helpers `0x02C460` for a triplet and `0x02C580` for one value; server `0x002510`, `0x002A10`).
- **Q(v)**: ((v.x × v.x) + (v.y × v.y)) + (v.z × v.z), every product and sum rounded to binary32 (helper `0x024860`, server `0x0021B0`). It is a squared length.
- **N(v)**: normalisation (helper `0x0240F0`, server `0x001FB0`): L = binary32 of the runtime's precise square root (`0x28D9C8`) of the binary64 widening of Q(v); r = binary32 (1.0 ÷ L); each component is then multiplied by r in binary32. A zero vector gives infinite r and NaN components; every use below is guarded so that v is non-zero.
- **cos_f(x)**, **sin_f(x)**, **acos_f(x)**, **asin_f(x)**: the runtime's precise binary64 routines (`0x28D7A4` cosine, `0x28D9C2` sine, `0x28DB32` arccosine, `0x28D9BC` arcsine) applied to the binary64 widening of a binary32 argument, the result narrowed to binary32 (round to nearest). Wrappers: `0x00E420` (cosine), `0x024B50` (sine), `0x0C14F0` (arccosine); server `0x0024A0`, `0x0024C0`, `0x148B00`.
- **Approach(p, g, n, r)**: the repeated-approach helper `0x011D40` (server `0x12E710`). It repeats n times (none when n ≤ 0, signed): the binary32 at p becomes ((g − p) × r) + p, each operation rounded to binary32, where g is re-read each time but never changes in any use below. The loop is unrolled by four; the unrolling does not change the result.
- **π**: binary64 `0x400921FB54442D18`. **180.0**: binary64 `0x4066800000000000` unless stated binary32 (`0x43340000`).
- **Block index** of a signed 64-bit v: the low 32 bits of the signed quotient of v by 65536 truncated toward zero, minus one when v is negative, as a signed 32-bit value (helper `0x02F040`, applied to a triplet, using the runtime division `0x28D9E0`).

## 1. Course steering (client `0x21FF4E`–`0x22063A`; server `0x145D0E`–`0x1463FA`)

### 1.1 Placement and gate

The block lies in the second pass over L of the world update `0x20C510` (server `0x1322D0`), after the current position and yaw are saved and before the target position is integrated (integration starts at `0x22063F`). It runs only when world byte `+0xB4` is 0 (`0x21FF39`–`0x21FF48`); otherwise the whole block is skipped. Objects are visited in ascending key order and a null entry is skipped before this block. Each object is steered with the target position it had before this update's integration.

The state byte `+0x70` is tested in the order 1, 0, 2, 3 (`0x21FF51`, `0x220037`, `0x22018D`, `0x2202F2`). Any other value (4 to 255) skips the block: velocity, target yaw and state are unchanged.

No routine in this block writes home `+0x38`, heading `+0x50` or destination `+0x58`.

### 1.2 Waypoint terms used by states 0, 1 and 2

- **h**: binary32 of ((binary64 of the sign-flipped heading) × π) ÷ 180.0. The heading is negated in binary32 (sign mask at `0x345F70`), widened, multiplied by π and divided by 180.0 in binary64, then narrowed (`0x22003F`–`0x22006A`; state 2 at `0x220195`–`0x2201C0`).
- **Offset for state 0**: the triplet (sin_f(h) × −50.0, cos_f(h) × −50.0, 20.0), the products in binary32, −50.0 being binary32 `0xC2480000` (image `0x2FCEA4`), 20.0 binary32 `0x41A00000`. The cosine is evaluated before the sine.
- **Offset for state 2**: the same with +50.0 (binary32 `0x42480000`, image `0x2FCD7C`) in place of −50.0.
- **Waypoint W** (states 0 and 2): home + T(offset), per component, the addition 64-bit and wrapping (`0x02C800`). For state 1 the waypoint is home itself.
- **d** = F(W − target position), the subtraction 64-bit and wrapping per component (`0x02C7A0`), and q = Q(d).

### 1.3 State 1 (`0x21FF59`–`0x220032`)

1. d = F(home − target position); q = Q(d).
2. If 0.001 > q (binary32 `0x3A83126F`, image `0x2FD870`; an ordered strict comparison): velocity becomes (+0.0, +0.0, +0.0) and the state becomes **2** (`0x21FFCA`).
3. Otherwise, if q > 100.0 (binary32 `0x42C80000`, image `0x2FD0B4`; ordered strict): velocity becomes N(d) with each component then multiplied in binary32 by 10.0 (binary32 `0x41200000`).
4. Otherwise (0.001 ≤ q ≤ 100.0): velocity becomes d itself, unnormalised.
5. Then the target-yaw tail of 1.6 with heading as the goal.

### 1.4 State 0 (`0x22003F`–`0x220188`)

1. d = F(W₀ − target position) with W₀ the state-0 waypoint; q = Q(d).
2. If 0.001 > q: velocity becomes (+0.0, +0.0, +0.0) and the state becomes **1** (`0x220164`).
3. Otherwise velocity becomes N(d) × 10.0 per component. There is no 100.0 threshold in this state.
4. Then the tail of 1.6 with heading as the goal.

### 1.5 State 2 (`0x220195`–`0x2202ED`)

Identical to state 0 with the state-2 waypoint W₂, except that the arrival write makes the state **3** (`0x2202C0`).

### 1.6 Target-yaw tail of states 0, 1 and 2 (`0x21FFFF`–`0x22002B`, then `0x220623`–`0x22063C`)

After the state's own writes (including in the update in which the state changed): g = A(target yaw, heading, 1.0), where A is the angle helper of Section 2; then Approach(target yaw `+0x34`, g, step, 0.005) with rate binary32 `0x3BA3D70A`. A is evaluated once, before the repetitions.

### 1.7 State 3 (`0x2202FA`–`0x22063A`)

1. **Horizontal course.** d = F(destination − target position); then d.z is set to +0.0; q = Q(d) (a horizontal squared length).
2. **Arrival.** If 0.001 > q: velocity becomes (+0.0, +0.0, +0.0) (all three components), the state becomes **0** (`0x220381`), and step 3 is skipped.
3. **Horizontal velocity** (not on arrival). If q > 900.0 (binary32 `0x44610000`, image `0x2FD874`; ordered strict), d becomes N(d) × 30.0 per component (30.0 binary32 `0x41F00000`); otherwise d is kept. Then Approach(velocity x, d.x, step, 0.001) and afterwards Approach(velocity y, d.y, step, 0.001), rate binary32 `0x3A83126F`. Velocity z is not touched here.
4. **Ground height** (on both paths, `0x220402`–`0x220463`). Column coordinates cx and cy are the target position's x and y each divided by 65536 as signed 64-bit values, truncating toward zero, keeping the low 32 bits (helper `0x02F570`, which calls the runtime signed 64-bit division `0x28D9E0`). Note that this is truncation, not the block index: values in (−1, 0) world units give column 0. The column record is fetched by `0x0347A0` with (world, cx, cy, 0): it is absent when cx or cy is negative or not below 2²⁴; otherwise the zone at zone coordinates (cx ÷ 256, cy ÷ 256) is fetched by the zone lookup `0x034D10`, and when that zone exists the record is the 32-byte entry at index ((cx mod 256) + 256 × (cy mod 256)) of the array whose pointer is at zone `+0xA8`. With the record present, G is the 32-bit wrapping sum of its signed 32-bit fields `+0x10` and `+0x1C` (readers `0x0348B0`, `0x034B70`), replaced by 0 when that sum is negative (sign of the 32-bit sum). With the record absent, G = 0. (In the unstuck routine, `0x20AF69`–`0x20AF77`, these two fields are the lowest stored block height of the column and the count of stored blocks, so G is the height just above the column's stored run.)
5. **Vertical goal.** With V(k) = the 32-bit value G + k (wrapping), sign-extended to 64 bits and multiplied by 65536:
   - if target position z < V(100) (signed 64-bit, strict; helper `0x02C860`), the vertical goal is +10.0;
   - else if target position z > V(150) (strict; helper `0x03ACB0`), the goal is −10.0 (binary32 `0xC1200000`);
   - else the goal is +0.0.

   Then Approach(velocity z, goal, step, 0.001).
6. **Yaw goal** (`0x2204F3`–`0x22063A`). e = a copy of the velocity as it now stands, with e.z set to +0.0; p = Q(e).
   - If not p > 0.0 (ordered; a zero horizontal velocity), the target yaw is not changed and the block ends.
   - Otherwise e = N(e); its y component is clamped: set to 1.0 when greater than 1.0, then set to −1.0 when −1.0 is greater than it.
   - If e.x > 0.0 (ordered strict): goal = binary32 of ((binary64 of −acos_f(e.y)) ÷ π) × 180.0; otherwise goal = binary32 of ((binary64 of acos_f(e.y)) ÷ π) × 180.0. The negation is a binary32 sign flip. The order here is division by π, then multiplication by 180.0, unlike 1.2.
   - Then Approach(target yaw `+0x34`, A(target yaw, goal, 1.0), step, 0.001).

### 1.8 Summary of the cycle and derived observations

- The transitions are 1 → 2 → 3 → 0 → 1. A constructed object starts in state 1 (held contract, Section 1.2). The waypoints are: state 1 home; state 2 home plus 50 world units along (sin h, cos h) and 20 up; state 3 the destination, horizontally only, with the vertical speed driven toward a band 100 to 150 world units above the column height G; state 0 home plus 50 units along (−sin h, −cos h) and 20 up.
- No random draw, and no use of the step other than as the Approach repeat count, occurs in the block.
- Derived from the arithmetic, not observed at run time: in states 0 and 2 the speed is always 10 world units per second toward the waypoint, so the target moves by roughly 0.01 × step world units per update and the arrival test (squared distance below 0.001, i.e. about 0.0316 units) is met only when a step happens to land within that radius. With a constant step the target can oscillate about the waypoint without arriving. State 1 slows down within 10 units because its velocity equals the remaining offset.

## 2. The angle helper and the yaw follow

### 2.1 Angle helper A(a, b, t) (`0x023F70`; server `0x1306D0`)

Arguments: three binary32 values; the result is returned as binary32. In order:

1. α = binary32 of ((binary64 of (a ÷ 180.0 in binary32)) × π). The division is binary32 by binary32 180.0 (image `0x2FCE5C`); the multiplication is binary64; the narrowing rounds to nearest.
2. ca = cos_f(α), then sa = sin_f(α), each kept as its exact binary64 widening.
3. β from b exactly as in step 1; cb = cos_f(β), then sb = sin_f(β), widened likewise.
4. s = binary32 of ((sb × ca) − (cb × sa)), the products and the subtraction in binary64. The products of two binary32 values are exact in binary64; the subtraction rounds in binary64; the narrowing rounds again.
5. If s > 1.0, s = 1.0; otherwise, if −1.0 > s, s = −1.0 (ordered comparisons; a NaN passes through unchanged).
6. r = asin_f(s), kept as its binary64 widening.
7. k = ((binary64 of t) × 180.0) ÷ π, in binary64.
8. Result = (binary32 of (r × k), the product in binary64) + a, the addition in binary32.

In exact arithmetic the result is a + δ with δ, in degrees, the arcsine of sin(b − a), so δ lies in [−90, 90] for t = 1.0. It equals b modulo 360 only when b − a, reduced to (−180, 180], lies within [−90, 90]; otherwise the helper's value lies on the other side of a (a derived property, not a run-time observation). The helper makes no random draw and has no side effects.

### 2.2 The yaw follow (`0x2206FD`–`0x22074C`; server `0x1464BD`–`0x14650C`)

In every world (not gated by world byte `+0xB4`), after the target position and the current position have been advanced (held contract, Section 6.2 items 1 to 3): g = A(current yaw `+0x98`, target yaw `+0x34`, 1.0); then Approach(current yaw `+0x98`, g, step, 0.01) with rate binary32 `0x3C23D70A`. A is evaluated once per object per update, before the repetitions. No draw occurs.

## 3. Creature placement on objects (client `0x21F6DA`–`0x21F962`; server `0x14549A`–`0x145722`)

### 3.1 Placement and gate

The block is part of the per-creature body of the world update's creature loop, after the creature's movement sub-steps, and before the second pass over L. It therefore reads each object's current pose as left by the previous update's follow (or as set by the copy producer in this update). Conditions, in order:

1. World byte `+0xB4` is 0 (`0x21F6DA`); otherwise the block is skipped.
2. The creature's signed 64-bit value at `+0x1D30` is not 0 (both halves, `0x21F6E7`–`0x21F6F9`).
3. That value is a key of L (find `0x077D90` compared with the end, `0x21F70C`–`0x21F72E`). Otherwise the block is skipped.

Which creatures reach `0x21F6DA` is decided by earlier parts of the per-creature body that were not traced here. Three locals used below are established on every path to `0x21F6DA` (no jump enters the range from before their writes): the creature's size triplet pointer (creature `+0x80`, written at `0x219137`), its position pointer (creature `+0x10`, at `0x21B44B`) and a pointer to the binary32 at creature `+0x30`, the third component of the triplet at `+0x28` (at `0x21D7CC`).

### 3.2 Inputs fetched

- The model M is entry `0xA03` of the world's model table at world `+0x1C` (`0x0120C0`, which returns null for an index out of range). **There is no null test**: an absent model faults on the first size read.
- The object O is fetched from L by insert-or-find (`0x068AD0`) under the same key, three times (`0x21F75A`, `0x21F8AC`, `0x21F93B`); the key is present, so nothing is inserted. **There is no null test of the object pointer.**
- The position list: entry 0 of the ordered integer-keyed map at model `+0x28`, obtained by its index-or-insert routine `0x0EC1F0` (which would insert an empty entry 0 when absent). The model loader appends to this list, in the order x outermost, then y, then z, the integer coordinates of every voxel equal to (255, 0, 0), by appending at the end of the list (`0x11BAF1`–`0x11BC40`, append routine `0x201EB0`). The block uses only the **first** entry, i.e. the first such voxel in that order.

### 3.3 Computation

1. c = cos_f(θ) then s = sin_f(θ), with θ = binary32 of (current yaw `+0x98` × 0.017453292), binary32 `0x3C8EFA35` (rotation helper `0x024610`). The angle is **not** sign-flipped (the collision test flips it).
2. tx, ty, tz = binary32 of the model sizes at `+0x44`, `+0x48` and `+0x4C` (signed 32-bit, round to nearest), each multiplied in binary32 by −0.5 (binary32 `0xBF000000`, image `0x2FCE8C`).
3. If the position list is empty (`0x077230`), steps 4 to 7 are skipped and only step 8 runs.
4. zo = binary32 of ((size z × 0.5) + 0.001), where size z is the binary32 at creature `+0x88`, 0.5 is binary32 `0x3F000000` and 0.001 is binary32 `0x3A83126F`; Z = (0, 0, T(zo)).
5. p = the first list entry converted per component from signed 32-bit to binary32 (`0x187F30`). Its model-frame image, with every product and sum rounded to binary32 and association as written:
   - X = ((−s × p.y) + (c × p.x)) + ((−s × ty) + (c × tx));
   - Y = ((c × p.y) + (s × p.x)) + ((c × ty) + (s × tx));
   - Z' = p.z + tz.

   (The computation is a 4 × 4 binary32 matrix built from identity, the rotation and then the translation by (tx, ty, tz), applied to p with helpers `0x023E70`, `0x024610`, `0x024A60`, `0x0248A0`. Its other terms are exact zeros and its final division is by exactly 1.0, so they change at most the sign of a zero, which T removes.)
6. N = (O current position + T(X, Y, Z')) + Z, the additions 64-bit and wrapping per component.
7. If Q(F(creature position − N)) > 1.0 (binary32 `0x3F800000`; ordered strict), the creature position `+0x10` becomes N (all three 64-bit components). Otherwise it is unchanged.
8. In every case reaching this point, the 32-bit value at creature `+0x30` becomes a raw copy of O's current yaw `+0x98`.

No other field is written: the creature's velocity, flags and the object's carried set `+0x78` are untouched by this block. No random draw occurs, and the step is not used.

## 4. The companion spawn record and its creature

### 4.1 Construction in zone generation (client `0x1BAA1B`–`0x1BAB4A`; server `0x0EE18B`–`0x0EE2BA`)

Immediately after an object template's destination is written, a 4336-byte record is allocated and constructed by `0x1ADB20`. That constructor's values that matter here: binary32 `+0x08` = 200.0, `+0x28` = 1, `+0x2C` = 4, 16-bit `+0x30` = 0, `+0x34` = 1, `+0x38` to `+0x4F` = 0 (including the signed 64-bit creature identifier at `+0x48`), byte `+0x50` = 0, `+0x54` = 0, byte `+0x58` = 0, `+0x5C` = 0, `+0x60` = `+0x64` = −1, `+0x74` an embedded appearance block whose 16-bit field at its `+0x06` (record `+0x7A`) is 0 (`0x028750`), `+0x109C` = null, `+0x10C8` = −1, `+0x10E0` = 0 (64-bit), byte `+0x10E8` = 0, and an empty string at `+0x10CC`.

The zone generation then writes, in order: `+0x28` = 3; byte `+0x30` = `0x88`; `+0x10` = the template's target position (3 × 64-bit copy); `+0x2C` = 4; `+0x34` = the 32-bit value at `+0x24` of a structure the layout routine holds (not traced); binary32 `+0x08` = 512.0 (`0x44000000`); `+0x54` = the template's target yaw; `+0x109C` = a new 12-byte behaviour object (constructor `0x0DC2D0`) whose list at its `+0x04` receives one new 4-byte element (constructor `0x0C8130`, appended by `0x0860B0`); and the 64-bit `+0x10E0` = K(zone x, zone y, n), where zone x and zone y are the signed 32-bit values at zone `+0x60` and `+0x64` (captured at `0x1B02D5`–`0x1B02EE`), n is the count of the zone's template list `+0x24` before the template is appended, and K is `0x1C0120`: its three 32-bit arguments are sign-extended to 64 bits and combined as ((second shifted left by 16) + first) shifted left by 8, plus third, all wrapping in 64 bits. The record is appended to the zone's spawn list `+0x18`, then the template to the template list `+0x24`.

No random draw occurs in `0x1BAA1B`–`0x1BAB64` or in the helpers it calls.

### 4.2 Later generation steps applied to the record (zone generator `0x1E4850`; server at −`0xCC220`)

For each index i of the zone's spawn list `+0x18` in order (`0x1ED2CC` onward), a record whose `+0x48` is 0 (both halves) receives `+0x48` = K(first generator argument, second generator argument, i) (`0x1ED2D6`–`0x1ED308`). The two generator arguments (stored at `0x1E488F`–`0x1E4895`) are the same two values that the template identifier loop passes to K at `0x1EC61E`–`0x1EC62C`. The creature identifier of the companion is therefore keyed by its spawn-list index i, while its `+0x10E0` key equals the template's identifier (template-list index n); the two are generally different values.

Then the routine `0x03F7C0` is called with (record `+0x2C`, record `+0x74`, 0) (`0x1ED32F`). It makes random draws and sets bits of the field at record `+0x7A`; it was **not audited** (Section 8).

### 4.3 The spawn pass that creates the creature (client `0x20FB48`–`0x210453`; server `0x135908`–`0x136213`)

It runs inside the same per-zone loop as the copy producer of the held contract, under the same gate (world byte `+0xB4` is 0 and the world name is not empty, `0x20F948`–`0x20F962`): for each zone of the player-zone set, the zone's templates are processed first, then each record R of its spawn list `+0x18` in index order. It precedes the creature loop of the same update, so a creature created here is updated, and placed per Section 3, in the same update. For each R, in order:

1. If R `+0x40` ≠ R `+0x44`, R is skipped unless R `+0x40` ≤ v ≤ R `+0x44`, where v is the world's 32-bit value at `+0x80015C` (not traced). For the companion both are 0, so this test is not applied.
2. R is skipped when a creature with identifier R `+0x48` exists in the creature map (lookup `0x02F000`).
3. If R `+0x38` < 0 it is set to 0. If R `+0x38` is then not 0 and R `+0x3C` equals the world's value at `+0x800160`, R is skipped. For the companion `+0x38` is 0.
4. The player list is scanned in order; at the first player p with (R `+0x08` × R `+0x08`, binary32) > Q(F(p position − R `+0x10`)) (ordered strict) the scan stops and creation proceeds. With no such player R is skipped. For the companion the threshold is 262144.0, i.e. within 512 world units of the template's original target position.
5. A special branch (`0x20FCFE`–`0x20FFB4`) is taken only when the region record returned by `0x087DA0` for (zone x ÷ 8, zone y ÷ 8) (divisions truncating toward zero) exists, its `+0x34` is 5, and bit `0x1000` of R's 16-bit `+0x7A` is set. It reseeds the runtime generator with (R `+0x30` as 32-bit + R `+0x48` low 32 bits) and makes draws. Whether a companion record takes it depends on Section 4.2's unaudited step.
6. A 7776-byte creature is allocated and constructed by `0x03B690` with identifier R `+0x48`, and stored in the creature map under that key. Fields copied that concern this contract: position `+0x10` = R `+0x10`; creature `+0x30` (the yaw component of `+0x28`) = R `+0x54`; byte `+0x60` = low byte of R `+0x28` (3 for the companion); `+0x64` = R `+0x2C`; byte `+0x140` = R byte `+0x30` (`0x88`); byte `+0x141` = R byte `+0x31`; `+0x190` = R `+0x34`; `+0x1C0` = R `+0x10`; `+0x1B0` = (zone x, zone y, i) as three 32-bit values; `+0x1D2C` = R `+0x08` (512.0); **`+0x1D30` = R `+0x10E0`** (64-bit); byte `+0x1D38` = R byte `+0x10E8`; and, because R `+0x109C` is not null, `+0x13E4` = the result of the behaviour object's second virtual method (`0x0DC400`, which copy-constructs a new 12-byte behaviour object). Many other fields are copied; they were not audited here.
7. If bit `0x200` of R `+0x7A` is set, the size triplet `+0x80` is doubled (binary32 × 2.0). Then position z += T(binary32 of ((size z × 0.5) + 0.1)), with 0.1 binary32 `0x3DCCCCCD` (image `0x2FCB7C`).

The creature constructor `0x03B690` does not itself write `+0x1D30`; no other store to creature `+0x1D30` was found in the client listing. The companion is thus keyed to its object only through `+0x1D30`, and its key is a template identifier, which is also the object's key in L. (Reconstruction lead: the reconstruction calls this creature the airship's pilot.)

Random draws in this pass occur only on the special branch of step 5 (direct draws at `0x20FF14` and `0x20FF25`, plus nested draws in `0x1C7730`, which that branch calls). No other draw is reachable from the creation path of steps 6 and 7 through the recorded call graph.

## 5. The unstuck routine's object part (client `0x20AB30`, part from `0x20B03F`; server `0x1308B0`, part from `0x130DBF`)

### 5.1 Reach

The routine is called from the world update at `0x21B47E` (server `0x14123E`), before the creature's movement sub-steps, with (world, creature), when world byte `+0xB4` is 0 or the creature is the world's creature at world `+0xB8`, and in both cases only when creature byte `+0x60` is not 6 (`0x21B456`–`0x21B47E`). Inside, nothing is done when 0.0 > the binary32 at creature `+0x16C` (an ordered strict comparison; a NaN does not skip). Otherwise a terrain part runs first (`0x20AB70`–`0x20B039`, not audited here); it either finds no solid block and falls through to the object part, or performs one terrain move (creature z rewritten from the block index of (z − T(size z × 0.5)), and creature `+0x1180` set to 0) and then jumps to the object part. The object part therefore runs exactly once per call, after which the routine returns.

### 5.2 Per object, in ascending key order of L

For each entry (the successor walk is inline):

1. The object pointer is tested for null first; a null entry is skipped (unlike the collision test).
2. The creature position S is read (its value after any move by an earlier object of this call). d = F(S − O current position); the object is skipped when Q(d) > 4096.0 (binary32 `0x45800000`, image `0x302B80`; ordered strict), i.e. beyond 64 world units.
3. The model is entry `0xA03` of the model table at world `+0x1C`; the object is skipped when the table holds no such index (byte length of the table, with the low two bits cleared, not greater than `0x280C`) or the model pointer is null. Sizes: Mx, My, Mz = model `+0x44`, `+0x48`, `+0x4C` (signed 32-bit); hx, hy, hz = binary32 of each × 0.5.
4. **Model frame.** θ = binary32 of ((sign-flipped current yaw `+0x98`) × 0.017453292); c = cos_f(θ) then s = sin_f(θ). The centre in the model frame is P = (((c × d.x) + (−s × d.y)) + hx, ((s × d.x) + (c × d.y)) + hy, d.z + hz), every operation binary32 (the code is an inlined matrix product whose extra terms are exact zeros and whose final division is by exactly 1.0; they change at most the sign of a zero).
5. **Box.** e = 0.5 × creature size triplet `+0x80`, `+0x84`, `+0x88` (binary32). lo = block index of T(P − e), hi = block index of T(P + e), per component, the subtraction and addition in binary32.
6. **Voxel test.** A voxel (x, y, z) is solid when any of its three bytes is non-zero. Inside the grid (each coordinate ≥ 0 and below Mx, My, Mz respectively) it is the 3-byte entry at offset 3 × (((z × My) + y) × Mx + x) of the voxel array at model `+0x30` (32-bit wrapping arithmetic); outside, it is the image's shared out-of-grid cell (client image `0x36B340`, server `0x183DFC`), read inline.
7. **Hit scan.** x from lo.x to hi.x (outermost), then y from lo.y to hi.y, then z from lo.z to hi.z. After each complete z column, if any voxel of that column was solid, the scan ends and step 8 runs. When no voxel of the box is solid the object is skipped.
8. **Candidate escapes**, tested in this order. Each candidate is available when every voxel of the box shifted by one voxel along its axis is not solid (the order of this emptiness scan does not affect its result):
   - **+x**: x from lo.x + 1 to hi.x + 1, y and z over the box; candidate centre (((binary32 lo.x) + 1.05) + (size x × 0.5), P.y, P.z);
   - **−x**: x from lo.x − 1 to hi.x − 1; centre (((binary32 hi.x) − 0.05) − (size x × 0.5), P.y, P.z);
   - **+y**: y from lo.y + 1 to hi.y + 1, x and z over the box; centre (P.x, ((binary32 lo.y) + 1.05) + (size y × 0.5), P.z);
   - **−y**: y from lo.y − 1 to hi.y − 1; centre (P.x, ((binary32 hi.y) − 0.05) − (size y × 0.5), P.z).

   Here 1.05 is binary32 `0x3F866666` (image `0x302A50`), 0.05 is binary32 `0x3D4CCCCD` (image `0x2FCDB4`), the integer-to-binary32 conversions round to nearest, size x and size y are creature `+0x80` and `+0x84`, and every operation is binary32. There is no z candidate. For each available candidate C, its distance is ((Δy × Δy) + (Δx × Δx)) + (Δz × Δz) with Δ = C − P per component in binary32 (only one component is non-zero). The best distance starts at −1.0. The first available candidate is taken. A later one replaces it only when its distance is strictly less (ordered). Equal distances keep the earlier candidate.
9. **No escape.** When the best distance is still less than 0.0 (no candidate available), nothing is written for this object.
10. **Move.** Δ = best − P per component (binary32; Δz is +0.0 or −0.0). With φ = binary32 of (current yaw `+0x98` × 0.017453292), **not** sign-flipped, c′ = cos_f(φ) then s′ = sin_f(φ), the world offset is u = (((−s′ × Δy) + (c′ × Δx)), ((c′ × Δy) + (s′ × Δx)), Δz), every operation binary32 (zero terms omitted as above). The creature position `+0x10` becomes S + T(u) per component, 64-bit and wrapping. No other field is written; the walk then continues with the next object using the new position.

No random draw occurs anywhere in the routine (no draw instruction in it, and none reachable from its callees through the recorded call graph). The step is not used.

## 6. Random draws (summary)

- Course steering (1), the angle helper and yaw follow (2), placement (3), the companion record's construction (4.1), the identifier assignment (4.2) and the unstuck routine (5): **no draws**. This was checked by searching each instruction range for direct calls of the runtime `rand` and `srand`, and by following the recorded call graph of every helper called (depth 12). Calls through virtual tables are not recorded; the only one met, the behaviour copy `0x0DC400`, has no draw.
- Generator step `0x03F7C0` (4.2), applied to every spawn record including the companion: **draws, not counted**.
- Spawn pass (4.3): `srand` and draws only on the special branch of step 5, plus nested draws in `0x1C7730` there.

## 7. Client/server agreement

Compared instruction for instruction after masking absolute addresses, with every masked constant read from both images and found equal:

- At −`0xDA240`: `0x21F6DA`–`0x220A80` (placement, steering, integration and follow; 1183 instructions, no differences), `0x21B456`–`0x21B483` (unstuck call site), `0x20F942`–`0x210499` (copy producer and spawn pass; 738 instructions).
- At −`0xCC890`: `0x1BA9C0`–`0x1BAB64` (template tail and companion record). At −`0xCC220`: `0x1ED2CC`–`0x1ED334` (spawn identifier assignment).
- The unstuck routine `0x20AB30` against server `0x1308B0` (1425 instructions): identical except data addresses, namely the shared out-of-grid cell and three shared block cells used by the terrain part.
- The model loader's red-voxel loop `0x11BAF1`–`0x11BC8A` against server `0x05ECC1` onward: identical.
- Helpers (client, server): A `0x023F70`/`0x1306D0`; Approach `0x011D40`/`0x12E710`; N `0x0240F0`/`0x001FB0`; cosine, sine and arccosine wrappers `0x00E420`/`0x0024A0`, `0x024B50`/`0x0024C0`, `0x0C14F0`/`0x148B00`; column lookup `0x0347A0`/`0x006100` and zone lookup `0x034D10`/`0x006290`; column readers `0x0348B0`/`0x0061D0`, `0x034B70`/`0x006250`; height comparisons `0x02C860`/`0x002D10`, `0x03ACB0`/`0x005750`; division by 65536 `0x02F570`/`0x005640`; list helpers `0x077230`/`0x130540`, `0x20AA50`/`0x130680`, `0x187F30`/`0x0C64B0`; triplet helpers `0x02C460`, `0x02C4A0`, `0x02C580`, `0x02C5B0`, `0x02C6E0`, `0x02C7A0`, `0x02C800`, `0x02C980`, `0x024860` and their server counterparts. All identical after masking.
- Server constants equal to the client's: 0.001 (`0x158858`), 100.0 (`0x1588E8`), 900.0 (`0x15885C`), ±50.0 (`0x158828`, `0x1587A0`), 0.0 (`0x158698`), ±1.0 (`0x1586DC`, `0x158794`), sign mask (`0x1587C0`), π (`0x158710`), binary64 180.0 (`0x158720`), binary32 180.0 (`0x158770`), ±0.5 (`0x1586D0`, `0x158790`), 0.017453292 (`0x1586AC`), 4096.0 (`0x173CC0`), 1.05 (`0x173C94`), 0.05 (`0x1586B8`), 72089.6 (`0x173CC8`), 0.1 (`0x1586BC`), 262144.0 (`0x15B23C`).
- Not instruction-identical in the listings: the map index-or-insert routine `0x0EC1F0` against its server counterpart `0x031020` (the client listing of it has undecoded gaps). Their equivalence is a lead, not verified.

## 8. Unresolved

- What generator step `0x03F7C0` writes into the companion record (notably bits `0x1000` and `0x200` of `+0x7A`), and hence whether the spawn pass's special branch or the size doubling applies to the companion, and how many draws that step makes.
- The meaning of the companion record's `+0x34` source, of the behaviour object and its element (types not traced), and of the many creature fields copied by the spawn pass that are not listed in 4.3.
- Initialisation of creature `+0x1D30` for creatures not created by the spawn pass (sub-constructors of `0x03B690`, network or save paths), and whether a stale value could match a key of L.
- Which creatures the per-creature body skips before `0x21F6DA`, and the server's values of world byte `+0xB4`, world `+0xB8` and the world name, which gate Sections 1, 3, 4.3 and 5.
- The unstuck routine's terrain part (its block solidity rule and box), audited here only as far as it hands over to the object part.
- The world values at `+0x80015C` and `+0x800160` and the region record kind read in 4.3.
- Equivalence of the client and server map index-or-insert routines.
- Calls through virtual tables other than `0x0DC400` were not followed for random draws.
