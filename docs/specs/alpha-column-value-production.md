Status: FINAL / STABLE / HELD

# Original column value: production at column creation, the fallback service, and its readers

## 0. Evidence and scope

The supported originals are the 32-bit x86 images `Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, both with preferred base `0x00400000`. Both hashes were recomputed at the start of this work on `binary/Cube.exe` and `binary/Server.exe` under the evidence root `Documents/disassembled alpha cube world`, and matched. Part-way through, those two files stopped being visible in `binary/`. Every later byte decode used `game-files/Cube.exe` and `game-files/Server.exe` instead, after their hashes were recomputed and found identical to the values above. Nothing under the evidence root was modified.

Instruction evidence came from `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. Call-site lists came from the matching `calls.csv` files. Constants, import names and static text were decoded from the image bytes. Decompiler output was used only to find leads, and every statement below was checked against the listed instructions.

Every address is an RVA in the named image, written "client X / server Y". Bit patterns are given in hexadecimal, as binary32 (`0x........`) or binary64 (`0x................`).

The earlier contract `docs/specs/alpha-template-stamping-markers.md` was used only to locate the column lookup, the column record and the fallback service. The facts it states that this contract relies on were checked again in the originals. The permitted reconstruction reference was **not** consulted.

Terms:

- **Column record**: a 32-byte element of a sector's column array. Its `+0x00` is a table pointer. `+0x04`, `+0x08` and `+0x0C` are binary32. `+0x10` is the lowest stored z, `+0x18` the cell-storage pointer and `+0x1C` the stored count. **The member** below means the binary32 at `+0x08`.
- **Column lookup** (client `0x0347A0` / server `0x006100`): takes (x, y, hint) and returns a column record or null. Its rules are restated in section 5.1.
- **Fallback service** (client `0x1C4DD0` / server `0x0F8B40`): the routine the stamping routine calls when no column record exists.
- **World**: the receiver object passed in ECX to the lookup, the fallback and the generators.
- **Draw**: one call through the imported `rand` (client import slot `0x2FC468` / server `0x1582F4`). **Reseed**: one call through the imported `srand` (client slot `0x2FC4B4` / server `0x15832C`). The image import tables name these functions as `rand` and `srand` of `MSVCR110.DLL`. The hint/name entries were decoded from the bytes (client `0x36358A` and `0x363670`, server `0x181484` and `0x18153C`). The generator's own algorithm lives in that library and is not established by these images.
- "r rem n" is the signed 32-bit remainder after sign-extending division, so its sign follows r.
- "trunc32" is conversion toward zero to a signed 32-bit integer (CVTTSS2SI or CVTTSD2SI).
- "tdiv k" is signed 32-bit division by a power of two k, truncating toward zero. The images implement it as add-bias-then-shift.

## 1. Summary of findings

1. Exactly two routines were found writing the member:
   - the column constructor, which writes binary32 `+0.0` (`0x00000000`);
   - the sector generator (client `0x1E4850` / server `0x118630`), which writes the fallback service's result once for every column of a newly created sector.

   No other writer was found (section 4.3 gives the census and its limits).
2. The sector generator makes no draw while producing the member. The value depends only on immutable "region point" objects created earlier by the point generator (section 3). The point generator does reseed and draw.
3. The fallback service is a deterministic function of (x, y) and of the region-point table's contents at the moment it is called. If every point it needs is present, it reproduces bit for bit the value a column record created for the same (x, y) holds. If any needed point is absent, or an index falls outside 0–1023, it returns 0.5 instead (section 2.4).
4. Client and server agree. The fallback, the noise routine, the point generator, the column lookup, the read-with-fallback wrapper and the opening of the sector generator through its column loop each match instruction for instruction once absolute addresses are masked. Every referenced constant decodes to the same bits in both images (section 7).

## 2. The fallback service (client `0x1C4DD0` / server `0x0F8B40`)

### 2.1 Interface

- Receiver in ECX: the world.
- Two 32-bit signed stack arguments, x then y, in world cell units.
- The callee releases them (`RET 0x8`).
- The result is a binary32 left on the x87 stack.
- No hint argument.
- It calls only the noise routine (section 2.5), twice, and the stack-cookie check.
- It makes no draws and writes no memory outside its own frame.

### 2.2 Inputs read from the world

The service reads a 1024 × 1024 table of point pointers. The entry for index pair (i, j), with 0 ≤ i, j < 1024, is the 32-bit pointer at world `+0x4000BC + 4 × (1024 × i + j)`. Index i comes from x and j from y.

Of each point it reads three fields: the 32-bit integers `+0x00` (point x) and `+0x04` (point y), and the binary32 `+0x10` (the **point value**).

### 2.3 Arithmetic, in order

1. **Index ranges**:
   - i0 = (x − 16384) tdiv 16384;
   - j0 = (y − 16384) tdiv 16384;
   - i1 = (x + 16384) tdiv 16384;
   - j1 = (y + 16384) tdiv 16384.

   Because the division truncates toward zero, x = 0 gives i0 = −1, while 0 < x < 16384 gives i0 = 0.
2. **First noise**: n1 = noise(a = binary64(y) × 0.0005, b = 3423.0), rounded to binary32. The constants are:
   - 0.0005: binary64 `0x3F40624DD2F1A9FC` (client `0x302B38` / server `0x173748`);
   - 3423.0: binary64 `0x40AABE0000000000` (client `0x31DE40` / server `0x173AD8`).

   Then ox = (n1 × 3.0) × 256.0 in binary32, with 3.0 = `0x40400000` (client `0x2FCD70` / server `0x158718`) and 256.0 = `0x43800000` (client `0x302AB8` / server `0x173C14`).
3. **Second noise**: n2 = noise(a = binary64(x) × 0.0005, b = 23421.0), rounded to binary32. 23421.0 is binary64 `0x40D6DF4000000000` (client `0x31DEB8` / server `0x173B50`). Then oy = (n2 × 3.0) × 256.0 in binary32.
4. **Sample point**: px = trunc32(binary32(x) + ox) and py = trunc32(binary32(y) + oy), both additions in binary32. The x-offset therefore comes from the noise taken along y, and the y-offset from the noise taken along x.
5. **If i0 > i1**, return 0.5 immediately. This cannot happen for any 32-bit x without overflow.
6. **First pass**: i ascends from i0 to i1 in the outer loop and j ascends from j0 to j1 in the inner loop. For each (i, j):
   - If i < 0, j < 0, i ≥ 1024 or j ≥ 1024, return 0.5 immediately.
   - If the table entry is null, return 0.5 immediately.
   - Otherwise:
     - form ex = (point x − px) × 65536 and ey = (point y − py) × 65536 as exact signed 64-bit integers;
     - convert each to binary32 through the x87 unit (one rounding, under the default round-to-nearest mode);
     - multiply each by 2^−16 (binary32 `0x37800000`, client `0x2FCD94` / server `0x15869C`);
     - square each in binary32 and add them as (y-term) + (x-term) in binary32;
     - trunc32 the sum to get the integer distance d.

   Scaling by 2^−16 is exact, so each scaled difference equals binary32 of the integer difference. The service keeps dmin, the smallest d. A later d replaces it only when strictly smaller, so ties keep the earlier entry. The point that gave dmin is used only to know that one exists.
7. **Second pass**: the same two loops in the same order, with the same rejection rules (any failure still returns 0.5). Two binary32 sums, W and S, start at +0.0. For each entry:
   - recompute d identically;
   - t = binary32(d − dmin), where the subtraction is 32-bit;
   - multiply t by 5.0e−7 (binary32 `0x350637BD`, client `0x31DCDC` / server `0x17394C`);
   - if the product is greater than 1.0 (binary32 `0x3F800000`, client `0x345DC0` / server `0x1586DC`), use 1.0 instead;
   - w = 1.0 − (that value);
   - W = W + w, then S = S + (point value × w), all in binary32.
8. **Result**: if W > 0.0, the result is S ÷ W in binary32. Otherwise it is 0.5 (binary32 `0x3F000000`, client `0x345DA0` / server `0x1586D0`). The point that set dmin has weight 1, so W ≥ 1 whenever step 8 is reached.

The result is a weighted mean of the point values of the up to 3 × 3 points around the jittered sample position. Each weight falls linearly with the squared-distance excess over the nearest point and reaches zero at 2,000,000 units of squared distance.

### 2.4 When the result is 0.5 instead of a mean

The service returns 0.5 whenever any (i, j) in its range is outside 0–1023 or has a null table entry. This covers:

- x or y equal to 0;
- x or y at or above 1023 × 16384;
- any position whose 3 × 3 neighbourhood of point cells has not been populated, or has been depopulated (section 3.4).

### 2.5 Noise routine (client `0x0C0EF0` / server `0x0D5D30`)

**Interface**: cdecl, two binary64 stack arguments a then b. It returns a binary32 value on the x87 stack (the binary64 result rounded through a 32-bit store). It calls only the imported cosine routine of `MSVCR110.DLL` (`libm_sse2_cos_precise`, through client `0x28D7A4`), twice. It makes no draws and writes nothing outside its frame.

**Arithmetic**:

1. ai = trunc32(a) and bi = trunc32(b). The next integers are formed as trunc32(binary64(ai) + 1.0) and trunc32(binary64(bi) + 1.0). Here 1.0 is binary64 `0x3FF0000000000000` (client `0x345DF8` / server `0x1588D0`).
2. **Lattice hashes** (all 32-bit wrapping):
   - h00 = ai + 57·bi;
   - h10 = (ai+1) + 57·bi;
   - h01 = ai + 57·(bi+1);
   - h11 = (ai+1) + 57·(bi+1).

   Each h is first replaced by h XOR (h shifted left by 13).
3. **Lattice values**: for each h, in 32-bit wrapping arithmetic, take h·h, multiply by `0xEC4D`, add `0x131071F`, multiply by h, subtract `0x2DF722F3`, and mask with `0x7FFFFFFF`. This gives v. Then r = 1.0 − binary64(v) × 2^−30 (binary64 `0x3E10000000000000`, client `0x302B20` / server `0x173728`). This yields r00, r10, r01 and r11.
4. **Fade weights**:
   - sa = (1.0 − cos((a − binary64(ai)) × C)) × 0.5;
   - sb = (1.0 − cos((b − binary64(bi)) × C)) × 0.5.

   C is binary64 `0x400921FB5A7ED197` (about 3.1415927, **not** the nearest binary64 to π), at client `0x302B48` / server `0x173788`. The 0.5 is binary64 `0x3FE0000000000000` (client `0x345DE8` / server `0x1586F0`). Both are computed in binary64.
5. **Blend**: in binary64, ((r00·(1−sa) + r10·sa) · (1−sb)) + ((r01·(1−sa) + r11·sa) · sb). Each product and sum is rounded in the order written. The result is then rounded to binary32.

## 3. Region points (the fallback's only data)

### 3.1 Point generator (client `0x1D7A70` / server `0x10B870`)

**Interface**: receiver is the world, with two 32-bit stack arguments i then j (`RET 0x8`). It returns the point pointer, or null.

**Existing entries**: if i or j is outside 0–1023, it returns null. If the entry already exists, it returns it with no draws.

**New entries**: otherwise it builds a new 0x1C-byte point. Let S be the world's 32-bit `+0x800188`. In order:

1. Reseed with 1024·j + 3·S + i + 0x108A (32-bit wrapping).
2. Allocate the point:
   - `+0x14` = 1024·j + i + S;
   - byte `+0x08` = 0.
3. **Flag F**: F is true exactly when i = ((trunc32(P0) tdiv 256) tdiv 64) and j = ((trunc32(P1) tdiv 256) tdiv 64). P0 and P1 are the world's binary32 values at `+0x8000F0` and `+0x8000F4`. The j test is skipped when the i test fails.
4. Draw r1. Path **A** is taken if (r1 rem 2) = 0 or F is true. Otherwise path **B** is taken.
5. **Path B**, four draws in order:
   - r2, then r3: `+0x0C` = (binary32(r3) × 0.1) ÷ 32767.0, plus 0.9 only when (r2 rem 2) ≠ 0;
   - r4, then r5: `+0x10` = (binary32(r5) × 0.1) ÷ 32767.0, plus 0.9 only when (r4 rem 2) ≠ 0.

   Then go to step 7.
6. **Path A**:
   - Draw ra: `+0x0C` = (binary32(ra) × 0.4) ÷ 32767.0 + 0.3.
   - Draw rb: `+0x10` = binary32(binary64(rb) × 0.4 ÷ 32767.0 + D3). This is computed in binary64 and rounded once to binary32 (CVTPD2PS). D3 is the binary32 0.4 widened to binary64.
   - If F is true, go to step 8.
   - Otherwise draw rc. If (rc rem 10) = 0:
     - byte `+0x08` = 1;
     - draw rd: `+0x10` = (binary32(rd) × 0.2) ÷ 32767.0 + 0.8;
     - draw re: `+0x0C` = (binary32(re) × 0.5) ÷ 32767.0.
   - Go to step 7.
7. **Drawn position**:
   - draw rx: `+0x04` = 16384·j + (rx rem 15360) + 512;
   - draw ry: `+0x00` = 16384·i + 512 + (ry rem 15360).

   The first of the two draws gives the y coordinate. Go to step 9.
8. **Fixed position** (F true): `+0x04` = trunc32(P1) and `+0x00` = trunc32(P0). No draws.
9. **Snap**: each coordinate c becomes 2048·(c tdiv 2048) + 1024. The x coordinate is snapped first, then y.
10. **Height noises**:
    - n3 = noise(a = binary64(i)·1.4 + binary64(S), b = (binary64(j)·1.4 + binary64(S)) + 843.0), rounded to binary32;
    - h = (n3 + 1.0) × 100.0 − 70.0, in binary32;
    - n4 = noise(a = binary64(i)·4.0 + binary64(S), b = (binary64(j)·4.0 + binary64(S)) + 843.0), rounded to binary32;
    - `+0x18` = trunc32(h + n4 × 30.0), in binary32.
11. **If `+0x18` > 0**, store the pointer in the table and return it.
12. **Otherwise**, set byte `+0x08` = 0, then:
    - If F is true: draw rf, set `+0x18` = (rf rem 50) + 20, store the pointer and return.
    - If F is false: set `+0x18` = max(`+0x18` − 100, −100) and draw rg.
      - If (rg rem 2) ≠ 0, draw twice more: the first draw gives `+0x0C` in the binary32 form of step 6 (× 0.4 ÷ 32767.0 + 0.3), and the second gives `+0x10` in the binary64 form of step 6.
      - Otherwise, draw twice more: `+0x0C` and then `+0x10` each = (binary32(r) × 0.1) ÷ 32767.0 + 0.9.

      Store the pointer and return.

**Draw count per new point**:

| Path | Draws |
| --- | --- |
| F true | 3, plus 1 more when `+0x18` ≤ 0 |
| Path B | 7, plus 3 more when `+0x18` ≤ 0 |
| Path A without F | 6, or 8 when (rc rem 10) = 0; plus 3 more when `+0x18` ≤ 0 |

**Constants used by the point generator**:

| Value | Bits | Client / server |
| --- | --- | --- |
| 0.1 | `0x3DCCCCCD` | `0x2FCB7C` / `0x1586BC` |
| 32767.0 | `0x46FFFE00` | `0x2FD338` / `0x158834` |
| 0.9 | `0x3F666666` | `0x2FD31C` / `0x158814` |
| 0.4 | `0x3ECCCCCD` | `0x2FD318` / `0x158810` |
| 0.3 | `0x3E99999A` | `0x2FCDC0` / `0x1586CC` |
| binary64 0.4 | `0x3FD999999999999A` | `0x31DD70` / `0x173A08` |
| binary64 32767.0 | `0x40DFFFC000000000` | `0x302A80` / `0x173850` |
| D3 | `0x3FD99999A0000000` | `0x31DD78` / `0x173A10` |
| 0.2 | `0x3E4CCCCD` | `0x2FCDBC` / `0x1586C4` |
| 0.8 | `0x3F4CCCCD` | `0x2FCDC8` / `0x1586D8` |
| 0.5 | `0x3F000000` | `0x345DA0` / `0x1586D0` |
| binary64 1.4 | `0x3FF6666666666666` | `0x31DD90` / `0x173A28` |
| binary64 843.0 | `0x408A580000000000` | `0x31DB70` / `0x173798` |
| binary64 4.0 | `0x4010000000000000` | `0x302B50` / `0x1715A8` |
| 1.0 | `0x3F800000` | `0x345DC0` / `0x1586DC` |
| 100.0 | `0x42C80000` | `0x2FD0B4` / `0x1588E8` |
| 70.0 | `0x428C0000` | `0x2FCE4C` / `0x158760` |
| 30.0 | `0x41F00000` | `0x2FCD78` / `0x158754` |

The point value read by the fallback is therefore one of these forms:

- 0.1·r/32767, with 0.9 added in some cases;
- the binary64 0.4·r/32767 + 0.4 form;
- 0.2·r/32767 + 0.8.

Each draw is a uniform library draw. Because each new point reseeds first, its contents depend only on (i, j), S, P0, P1 and the library generator, never on draws made earlier.

### 3.2 Who creates points

The only caller of the point generator is the region routine (client `0x1DA280` / server `0x10E080`), which takes 32-bit (rx, ry):

- It returns at once if either index is outside 0–1023, or if the region table entry (world `+0xBC + 4 × (1024·rx + ry)`) already exists.
- Otherwise it calls the point generator for every (rx + di, ry + dj), with di ascending from −2 to 2 in the outer loop and dj ascending from −2 to 2 in the inner loop.
- It then reseeds with S + 1024·ry + rx and continues building the region. That later work was not audited beyond its one fallback call (section 5.3).

The sector generator calls the region routine for (rx + dx, ry + dy), with dx ascending from −1 to 1 in the outer loop and dy ascending from −1 to 1 in the inner loop. Here rx = sx tdiv 64 and ry = sy tdiv 64, and (sx, sy) is the sector being created. These calls come **before** any column is written. The points in region cells rx − 3 to rx + 3 (and likewise for y) therefore exist when the column loop runs. Indices outside 0–1023 are never created.

### 3.3 World inputs S, P0 and P1

- **S** (`+0x800188`): the world initialiser (client `0x1A52E0` / server `0x0D83A0`) reseeds with its first stack argument, stores that argument at `+0x800164`, then draws once and stores (draw rem 100000) at `+0x800188`. This is the only instruction found writing `+0x800188` in either listing. A byte scan of the client image for the displacement `0x800188` found six uses, one of them this store. The source of the initialiser's argument was not traced.
- **P0 and P1** (`+0x8000F0` and `+0x8000F4`): the same initialiser stores binary32 `0x4B002080` (8396928.0) in each, and 0 at `+0x8000F8`. Several other client routines take the address of `+0x8000F0` (for example the instructions at client `0x087FE0`, `0x1FAEAD`, `0x201FB0` and `0x2050E6`) and may write through it; they were not audited. With the initial values, F is true only for (i, j) = (512, 512), whose fixed position snaps to (8397824, 8397824). Whether P0 and P1 change before that point is created is unresolved.

### 3.4 Point lifetime

- **Zeroing**: the world constructor (client `0x18EB00`, loop at `0x19335C` / server `0x0C8570`, at `0x0CCDCC`) zeroes both 1024 × 1024 pointer tables, the region table at `+0xBC` and the point table at `+0x4000BC`.
- **Storing**: the only instructions found storing a point pointer are the two in the point generator (client `0x1D7E66` and `0x1D7F39`). A byte scan of the client image for the displacement `0x4000BC` found exactly the 14 uses that the listing shows.
- **Removal**: client `0x1A4780` / server `0x0D78E0` takes (i, j). If the entry is non-null, it enters the two critical sections at world `+0x8000C0` and `+0x8000D8`, stores null, leaves them and frees the point. Its only caller (client `0x06A8A0`, call at `0x06AF25` / server `0x149550`) scans all (i, j). It removes the entry either when a local flag is set, or when no element of a list it walks lies within 4 region units of (i, j) on both axes. The list's identity and the full rule were not audited.
- **Mutation**: no instruction was found writing a point's fields after creation. The other routines that index the point table (client `0x07FAA0` and its callers, `0x1DA280`, `0x1EE4B0`, `0x1EEA60`, `0x1F7820`, `0x2024D0`, `0x077E10`, `0x1C4800`, `0x1EF040`, `0x1C5E20`, `0x1CADE0`, `0x1CB0D0`, `0x1F0720`) were inspected around their table accesses, and only loads from points were seen there. These routines were not audited in full.

A point that is removed and later regenerated is rebuilt by the same reseeded sequence. It is therefore identical, provided S, P0, P1 and the library generator behave the same.

## 4. Writers of the member

### 4.1 Column constructor (client `0x050BF0` / server `0x0133C0`)

**When**: it runs for each of the 65536 elements of a new sector's column array. The array is allocated as 0x200004 bytes by the sector constructor (client `0x2240F0` / server `0x148B60`), which stores the element base at the sector's `+0xA8`. The constructor is reached through the runtime's vector-construction helper; the listing does not mark it as a function.

**Effect**: it sets `+0x00` to the column table pointer (client `0x2FFD90`, whose only entry is `0x224700`). It stores 32-bit zero in `+0x04` through `+0x1C`, so the member holds binary32 `+0.0`.

### 4.2 Sector generator (client `0x1E4850` / server `0x118630`)

**Interface**: receiver is the world, with stack arguments sector indices (sx, sy).

**Order of operations**:

1. It returns without action if sx or sy is outside 0–65535 (signed).
2. It calls the region routine nine times (section 3.2).
3. It returns without action if the sector lookup (client `0x034D10`) already finds the sector.
4. Otherwise it constructs the sector, stores (sx, sy) at the sector's `+0x60`/`+0x64` and reseeds with S + 65536·sy + sx.
5. It calls client `0x077E10` and `0x19FC90` (neither audited).
6. It runs the column loop. Coordinate x ascends from 256·sx to 256·sx + 255 in the outer loop; coordinate y ascends from 256·sy to 256·sy + 255 in the inner loop. For each (x, y), the column is element (y rem 256) × 256 + (x rem 256) of the new sector. In order, the loop:
   - stores the result of client `0x1C4800` / server `0x0F8570`, called with (x, y), into `+0x04`;
   - stores **the fallback service's result for (x, y)** into the member;
   - stores the result of client `0x1EF040` / server `0x122E20`, called with (x, y), into `+0x0C`.

   Each result reaches memory by a single 32-bit x87 store of a value that is already binary32, so the bits are exactly the fallback's result.
7. Later work in the same routine fills the column cells (not audited).

**Draws**: none of the three column-loop routines draws or reseeds. Draws and reseeds happen in the region routine and in the reseed of step 4, which precede the column loop.

**Consequence**: for every column it creates, the member equals the fallback service applied to the same (x, y) with the same world, at a moment when every point the fallback needs exists, unless an index is outside 0–1023. Columns with x or y equal to 0, or at or above 1023 × 16384, hold 0.5.

The generator also writes the member's neighbours. Whether the new sector becomes visible to other threads before step 6 completes (when a reader could see `+0.0`) was not examined.

### 4.3 Census method and limits

**Method**:

- Every function in each listing that calls the column lookup or the sector lookup, or that loads a sector's `+0xA8`, was searched for 32-bit stores at displacement `+0x8`. Each hit was inspected. All other hits were stores into other objects, such as colour triples, list counts and feature-record copies.
- Every call site of the lookup was checked for `+0x8` accesses in the following 50 instructions.
- The column table holds no copy or serialisation entry.

**Limits**: a store reaching the member through a pointer computed in some other way (for example a bulk copy of a column array) would not be found by this method. No such copy was seen, but its absence is not proven.

## 5. Readers of the member

### 5.1 Common read pattern and the column lookup

Almost every reader does the same thing: call the column lookup, use the record's `+0x08` when it is non-null, and otherwise call the fallback service with the same (x, y). The rules of the lookup were re-read:

- It returns null if x or y is negative or ≥ 0x1000000.
- With a null hint, it finds the sector through client `0x034D10`.
- With a hint, it returns null unless 256 × hint`+0x60` ≤ x < 256 × hint`+0x60` + 256 and the same holds for y with `+0x64`.
- It returns null if no sector exists.
- Otherwise it returns the sector's `+0xA8` plus 32 × ((x rem 256) + 256 × (y rem 256)).

A wrapper (client `0x1C8350` / server `0x0FC140`) packages this pattern. Its receiver is the world and its stack arguments are (x, y, hint), released with `RET 0xC`. It returns `+0x08` when the lookup succeeds and the fallback's result for (x, y) otherwise, as binary32 on the x87 stack. A parallel wrapper for `+0x04` (client `0x1C8310`) uses client `0x1C4800` as its fallback.

Every decision a reader makes from V uses an ordered binary32 comparison, so a NaN never satisfies "greater than" or "less than". The fallback cannot produce NaN from finite inputs.

### 5.2 Readers found (client / server), with the decisions their own instructions show

Thresholds below are binary32:

| Value | Bits | Client RVA |
| --- | --- | --- |
| 0.2 | `0x3E4CCCCD` | `0x2FCDBC` |
| 0.25 | `0x3E800000` | `0x345D98` |
| 0.29 | `0x3E947AE1` | `0x31DD00` |
| 0.5 | `0x3F000000` | `0x345DA0` |
| 0.7 | `0x3F333333` | `0x2FD570` |
| 0.75 | `0x3F400000` | `0x2FCDC4` |
| 0.8 | `0x3F4CCCCD` | `0x2FCDC8` |

Unless stated otherwise, the downstream effects beyond the decision shown were not audited.

1. **Template stamping routine** (client `0x1F0CE0` / server `0x124540`):
   - It reads V once for the base cell (bx, by) with its zone hint: the read is at client `0x1F0E27`, with the fallback at `0x1F0E3B`.
   - V is compared twice, at client `0x1F1EE3` and `0x1F3284`. Both times control branches to the same target (client `0x1F4A7E`) unless V > 0.5. These are the two marker branches the earlier stamping contract describes, where that target skips the cell.
2. **Sector generator, direct reads** (client `0x1E52E9` and `0x1E6006` / server `0x1190C9` and `0x119DE6`): the generator itself, after writing, reads the member and `+0x04` of a column of the new sector. It passes them as binary32 stack arguments 6 and 5 of the cell-colour routine (item 8).
3. **Sector generator, lookup reads**, in its later stages:
   - client `0x1E6C38` (fallback `0x1E6C56`) / server `0x11AA36`: the value is stored in a local; its use was not traced;
   - client `0x1EA322` (fallback `0x1EA340`) / server `0x11E120`: if V > 0.8 and a previously computed local > 0.7, two draws follow. A local integer becomes 5 or 4 by whether the first draw's remainder by 2 is nonzero, and then 3 if the second draw's remainder by 4 is zero;
   - client `0x1EA6C3` (fallback `0x1EA6E1`) / server `0x11E4C1`: if V > 0.8, the column's `+0x04` value (with its own fallback) is read. If 0.2 > that value, the same local integer becomes 3;
   - client `0x1EB094` (fallback `0x1EB0B2`) / server `0x11EE92`: if 0.2 > V, one draw is made, and when its remainder by 4 is zero control jumps forward to client `0x1EBBA1`;
   - through the wrapper at client `0x1EBE1A` / server `0x11FBFA`: V > 0.8 selects sizes (1.0, 1.0, 8.0) and a 32-bit kind 51; otherwise sizes (2.0, 2.0, 8.0) and kind 50;
   - through the wrapper at client `0x1EBF32` / server `0x11FD12`: only if V > 0.2, a further noise test (> 0.5) and a draw (remainder by 8 equal to zero) gate a block;
   - through the wrapper at client `0x1EC105` / server `0x11FEE5`: only if V > 0.2 and a local integer > 0, a further noise test (> 0.7) and a draw (remainder by 10 equal to zero) gate a block;
   - through the wrapper at client `0x1EC83E` / server `0x12061E`: only if V > 0.75 is the column's `+0x04` value read; it is then compared against 0.25 to choose between two blocks.
4. **Client `0x1AF170` / server `0x0E28E0`**:
   - At client `0x1B24B1` / server `0x0E5C21` the fallback is called and its result is discarded.
   - At client `0x1BC010` / server `0x0EF780` the fallback's result is passed, with the `+0x04` fallback's result, as arguments 6 and 5 of the cell-colour routine.
   - Through the wrapper at client `0x1BC190` / server `0x0EF900`: V > 0.8 selects the integer 3, and otherwise 5. When a local count exceeds 1, it is passed to client `0x1DF960` together with two draws ((r rem 8) + 13 and (r rem 6) + 7).
5. **Colour routine** (client `0x1C7140` / server `0x0FAE90`), read at client `0x1C7476`: if V > 0.75, t = (V − 0.75) × 4.0 (4.0 at client `0x345E60`). The routine's colour triple is replaced by (colour × (1 − t)) + (the triple from client `0x1F9620`) × t.
6. **Colour routine** (client `0x1F9620` / server `0x12D030`), read at client `0x1F96F9`: if 0.5 > V, V × 2.0 (2.0 at client `0x345E10`) enters a colour mix with constants 1.0, 130.0, 170.0, 20.0 and 120.0. The mix itself was not audited.
7. **Client `0x1DF960` / server `0x113760`**, read at client `0x1DFA5B`. Two binary32 quantities are formed, each with one draw:
   - V × 50.0 + 120.0, plus binary32(draw rem 80);
   - V × 100.0 + 50.0, plus binary32(draw rem 50).
8. **Cell-colour routine** (client `0x1C56E0` / server `0x0F9450`), which receives V as its sixth stack argument. Its callers are:
   - client `0x1CB600` / server `0x0FF3F0` (lookup read at client `0x1CBBAC`);
   - client `0x1CBE00` / server `0x0FFBF0` (read at `0x1CC3E8`);
   - client `0x1CC510` / server `0x100300` (through the wrapper at `0x1CFF36`);
   - client `0x1AF170` / server `0x0E28E0`, and the sector generator (items 2 and 4).

   Inside, V is used in four ways:
   - if V > 0.8, a weight min((V − 0.8) ÷ 0.1, 1.0) is formed;
   - if 0.2 > V, a weight is scaled by V ÷ 0.2; when the weight is then positive, byte `+0x03` of the routine's output object (its first stack argument, initialised to the 32-bit value `0x04000000`) becomes 9;
   - if 0.3 > V, a weight min(1.0 − (V − 0.29) ÷ 0.01, 1.0) is formed (0.01 = `0x3C23D70A` at client `0x2FCDA0`), and that same output byte becomes 10;
   - the remaining blending and its final output were not audited.

### 5.3 Callers of the fallback that do not read the member

- **Client `0x088EE0`** (client only; no server counterpart):
  - It calls the fallback with a world at its own `+0x2E4`. It forms the integer −20 − trunc32(V × −60.0), with −60.0 = `0xC2700000` at client `0x2FCEA8`.
  - It pushes that integer next to the address of a static UTF-16 text at client `0x301B98`, which reads "  TEMP " (two spaces, `TEMP`, one space).
  - Just before, it scales the `+0x04` fallback's result by 100.0 and pushes it next to the text at client `0x301B84`, which reads " °C  HUM ".
  - Which text labels which number is not resolved from these instructions: the pushes are interleaved with unrelated calls. These texts are the only names the image supplies near the value. This contract assigns no meaning.
- **Client `0x19D640`** (client only):
  - The result is stored in a local.
  - It is later compared: V > 0.6 (`0x3F19999A` at client `0x2FD8DC`), with the `+0x04` value below 0.4, gates one block.
  - 0.2 > V gates another, which scales by V ÷ 0.2.
- **Region routine** (client `0x1DA280` at `0x1DAE89` / server `0x10EC89`): if the fallback's result for a position it computes exceeds 0.8, it stores a byte at a region entry. The byte is 4 when the `+0x04` fallback's result also exceeds 0.8, and 5 otherwise. Nothing is stored when the result is not above 0.8.
- **Client `0x1F56C0` / server `0x1290D0`**: the result is stored. Later, 0.2 ≤ V < 0.8 gates a block that sets a local to 54.

## 6. Does the fallback reproduce the stored value?

**Same value**: yes, bit for bit, when the fallback is called with the same world for the (x, y) of an existing column and every point in its 3 × 3 neighbourhood is present. The reasons are:

- the member was produced by the same routine;
- its only inputs are the point table entries;
- points are never modified after creation.

Two more conditions apply:

- the x87 rounding mode is the default round-to-nearest in both calls;
- the imported cosine returns the same results.

**0.5 instead**: the fallback differs from the value a column would hold, returning 0.5, when a needed point is absent. Examples are a position whose neighbouring regions have never been created, or whose points were removed by the cleanup of section 3.4. The same applies at the 0 and 1023 index edges, where the stored column value is also 0.5.

**Wrong hint**: a column that exists but is missed because a hint names a different sector also leads to the fallback. Its result equals the stored value under the conditions above.

## 7. Client and server agreement

Each pair below was extracted from its own listing and compared instruction by instruction, with absolute addresses in the range `0x400000`–`0x8FFFFF` masked. Opcodes, registers, immediates and frame and field offsets had to match exactly.

| Routine | Client | Server | Instructions | Mismatches |
| --- | --- | --- | --- | --- |
| Fallback service | `0x1C4DD0` | `0x0F8B40` | 326 / 326 | 0 |
| Noise routine | `0x0C0EF0` | `0x0D5D30` | 136 / 136 | 0 |
| Point generator | `0x1D7A70` | `0x10B870` | 318 / 318 | 0 |
| Column lookup | `0x0347A0` | `0x006100` | 76 / 76 | 0 |
| Read-with-fallback wrapper | `0x1C8350` | `0x0FC140` | 21 / 21 | 0 |
| Cell-colour routine | `0x1C56E0` | `0x0F9450` | 460 / 460 | 0 |
| Sector generator, first 241 instructions (through the column loop) | `0x1E4850` | `0x118630` | 241 / 241 | 0 |
| Region routine, first 76 instructions (through the 5 × 5 point loop) | `0x1DA280` | `0x10E080` | 76 / 76 | 0 |

**Constants**: every memory operand in the fallback (13), the noise routine (9) and the point generator (45) was decoded from both images in order. All binary64 constants have identical bits. All binary32 constants have identical first four bytes, which is the only part read. The only operands whose bytes differ are the `rand` and `srand` import slots, and both images name those imports `rand` and `srand`.

**Column constructor**: in both images the constructor is eight stores that zero `+0x04` through `+0x1C` after the table pointer.

**World initialiser**: both images store `0x4B002080` into `+0x8000F0` and `+0x8000F4`, and (draw rem 100000) into `+0x800188`.

**Reader sites**: twenty-one fallback or wrapper call sites in the client were paired with their server counterparts (the sites named in section 5). Each comparison covered a window from 25 instructions before to 45 after, under the same masking, with every referenced constant decoded. The only differences were operands naming the `rand` import slot. The server image has no counterparts for client `0x088EE0` and `0x19D640`. The server has 16 fallback call sites against the client's 18.

No behavioural difference between the images was found in anything this contract states.

## 8. Not closed

- The internal algorithms of the imported `rand`/`srand` and of the imported cosine. Bit-exact reproduction depends on both, and these images do not define them.
- The source of the world initialiser's seed argument. Whether anything rewrites S, P0 or P1 after initialisation; several routines take P0's address and were not audited.
- The exact removal rule of the point cleanup (the list it consults and the flag), and the region-removal call that precedes it (client `0x1A4800`).
- Whether a sector is visible to other threads while its column loop runs.
- The routines that produce `+0x04` (client `0x1C4800`) and `+0x0C` (client `0x1EF040`), the parts of the sector generator after the column loop, and the region routine after its point loop.
- The downstream effects of every reader decision in section 5, beyond the comparisons and arithmetic stated. In particular, the full output logic of the cell-colour routine and of the two colour routines.
- Which static text, if either, labels the fallback-derived number in client `0x088EE0`. No meaning is asserted for the value.
- Whether any column copy or bulk write reaches the member other than through the two writers found (section 4.3).
