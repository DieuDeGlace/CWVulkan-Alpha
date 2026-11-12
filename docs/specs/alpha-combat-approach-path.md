Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha combat far-route approach and path helpers

## Scope and evidence

This contract covers the movement helpers called by the far route of the created-actor first-child combat continuation (held contract `alpha-created-actor-combat.md`, section 5.2). These are the direct-approach test `0x209480`, the path request `0x1A9F90`, the search expansion step `0x1AAAB0`, the path rebuild `0x1A7C90` and the path-clearing helper `0x02EF10`, together with the world collision test and the per-tick path follower they depend on. The follower is described only as far as the listing was confirmed; see section 9.

Supported images (SHA-256 verified before use, both 32-bit, preferred base `0x00400000`):

- Client `binary/Cube.exe`, `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are client RVAs unless marked as server RVAs. Evidence came from the original instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, the call tables `decompiled/*/calls.csv`, and the image bytes. Decompiler output was used only to find leads. Branch conditions, widths, constants, loop bounds and operand order were confirmed in the listings. Where the client listing stops after a deallocation call that it wrongly treats as non-returning, the skipped bytes were decoded from the image. Static default blocks live in uninitialised data, so their contents were taken from the start-up routines that write them. No reconstruction material was needed or consulted.

Terms:

- **Subject** (S), **target** (T), **world** (W): as in the held contract. W is passed in the register `this`. S is the first stack argument unless stated otherwise.
- **Fixed point**: signed 64-bit, with 65536 units per world unit (one block).
- **Cell of a point**: helper `0x02F040`. Each coordinate is signed-divided by 65536 with truncation. If the 64-bit value is negative, one is then subtracted, even for an exact multiple. The low 32 bits are kept. This is the conversion meant by "cell" below unless another rule is stated.
- **To fixed (trunc)**: a binary32 value multiplied in binary32 by `65536.0` (client data `0x2FCE7C`), then truncated toward zero to signed 64-bit by the runtime helper `0x28D946`.
- **Half extents of S**: S `+0x80`, `+0x84`, `+0x88` (binary32 width, depth, height), each multiplied in binary32 by `0.5` (data `0x345DA0`) and taken to fixed (trunc). They are written hx, hy, hz.
- **Block word**: a block is 4 bytes. Its type is the low 5 bits of byte 3. Bit `0x40` of byte 3 is a separate flag.
- **Passable block**: a block whose type is `0` or `2`. Every other type blocks movement in every test in this contract.

All binary32 arithmetic rounds after each operation. Comparisons are ordered binary32 comparisons. Behaviour with NaN or infinite values, with 64-bit overflow and with invalid containers is not supplied.

## 1. Subject path state

The routines use the following subject fields:

| Offset | Width | Content |
|---|---|---|
| `+0x1408` | pointer | *best record*: a position in the record map (end = none) |
| `+0x140C` / `+0x1410` | map header / count | *record map*, keyed by a cell (three signed 32-bit, ordered lexicographically x, then y, then z, signed) |
| `+0x1414` / `+0x1418` | set header / count | *open set* of cells, same ordering |
| `+0x141C..+0x1424` | 3 × int32 | start cell |
| `+0x1428..+0x143F` | 3 × int64 | start point (fixed) |
| `+0x1440..+0x1457` | 3 × int64 | goal point (fixed) |
| `+0x1458` | binary32 | arrival radius |
| `+0x145C` | int32 | stall timer |
| `+0x1460` / `+0x1464` | list header / count | *path list*; each node holds next (`+0`), previous (`+4`) and a cell (`+0x08`, `+0x0C`, `+0x10`) |
| `+0x1468` / `+0x146C` | set header / count | *ignore set* of static-object pointers, ordered by unsigned pointer value |
| `+0x1478..+0x1480` | 12 bytes | value compared with sector-list entries in 4.3 |

A *record* is 24 bytes: `+0` cost g, `+4` estimate h, `+8` total f (all signed 32-bit), and `+0x0C..+0x14` the parent cell.

The far route writes the start point, the goal point and the arrival radius before it calls `0x1A9F90`; see section 5.2 step 8 of the held contract.

### Estimate

The estimate between two cells is computed as follows, with |dx|, |dy| and |dz| the absolute 32-bit differences. Let p = 10·|dx| + 4·|dy| when |dx| > |dy|, and otherwise 4·|dx| + 10·|dy|. The estimate is 2·(p + 10·|dz|), in wrapping 32-bit arithmetic (helper `0x1A9D40`). Inside `0x1A9F90` the same value is converted to binary32 and back by truncation, which changes nothing below 2^24.

### Record insertion

Insertion helper `0x1A9E50` takes a cell and a record. It does the following in order:

1. It stores the record under the cell in the record map, creating the entry if needed.
2. It inserts the cell into the open set. A cell already present is left as it is.
3. It makes the new entry the best record if there is no best record yet, or if the record's h is strictly less than the best record's h.

Lookup helper `0x1A9DA0` returns the stored record for a cell, or none. Membership helper `0x1A9CA0` tells whether a cell is in the open set.

## 2. Path-clearing helper `0x02EF10`

This helper takes the subject in `this` and draws no random numbers. It does the following in order:

1. It releases every record-map node, including each node's key storage, and resets the map to empty with count `+0x1410` = 0.
2. It does the same for the open set; `+0x1418` = 0.
3. It releases every path-list node and sets `+0x1464` = 0.
4. It sets the best record to "none", which is the map's end position.
5. It releases every ignore-set node and sets its count to 0.

The helper does not touch the start or goal points, the start cell, the arrival radius or the stall timer.

## 3. Path request `0x1A9F90`

This routine takes the world in `this` and the subject as its argument. It does the following in order:

1. It runs the path-clearing helper.
2. It sets the start cell `+0x141C..+0x1424` to the cell of the start point.
3. It sets the stall timer `+0x145C` to 0.
4. It inserts the *start record* for the start cell, using the insertion helper: g = **1**, h = the estimate from the start cell to the goal cell, f = **h** (not g + h), and parent = the start cell itself. The open set is then {start cell}, and the best record is the start cell.
5. It clears the ignore set again.
6. **Ignore-set fill.** This step runs twice: first for the start cell, then for the goal cell (the cell of the goal point). For a cell C:
   - The probe point Q is C × 65536 plus an offset. The offset is 32768 in x and y, and the fixed (trunc) value of binary32 (S `+0x88` × `0.5`) in z.
   - Sector coordinates are computed for x and y. The coordinate of Q is signed-divided by 65536 with truncation, and the result is then divided by 8, again truncating toward zero.
   - The 3 × 3 sectors from (sector − 1) to (sector + 1) are scanned. The x loop is outer and y inner, each running upward. A sector is skipped unless both coordinates are in [0, `0x200000`).
   - The sector's zone is found from the world's region table. For each axis the zone index is the sector index divided by 32; the region index is the zone index divided by 64 (region table at W `+0xBC`, 1024 × 1024); the zone pointer is at region `+0x10018`, a 64 × 64 table. The sector list is entry ((sector y mod 32) × 32 + (sector x mod 32)) of the 8-byte array at zone `+0xAC`. The first pass uses world helper `0x034D10` for this lookup and the second pass computes it inline. The result is the same.
   - Each list entry points to a static object. Object kinds `7`, `6` and `9` are skipped. Kinds `1`, `8`, `2`, `3` and `5` are considered only when the object byte `+0x30` is nonzero. All other kinds are considered.
   - The object footprint uses object `+0x24` and `+0x28` (binary32 sizes) and `+0x2C` (binary32 height). When the object's 32-bit `+0x20` is odd, the two horizontal sizes are swapped. The object half sizes are ox = fixed (trunc) of (size × 0.5), and likewise oy. The object height in fixed is oz = fixed (trunc) of the height (no halving). The object position is at `+0x08`, `+0x10` and `+0x18` (int64).
   - The object is *overlapping* when all of the following hold:
     - Q.x + hx ≥ obj.x − ox, and Q.x − hx < obj.x + ox;
     - the same two conditions in y, with hy and oy;
     - Q.z + hz ≥ obj.z, and Q.z − hz < obj.z + oz.

     Every term is a signed 64-bit value.
   - An overlapping object is inserted into the ignore set.

The routine performs no search expansion. It returns nothing.

## 4. Search expansion `0x1AAAB0`

This routine takes the world in `this` and the subject as its argument. It returns a byte. One call expands at most one cell.

### 4.1 Admission and selection

1. If the record count `+0x1410` is 0, it returns 0.
2. If the open-set count `+0x1418` is greater than `500` (unsigned), or the record count is greater than `20000` (unsigned), it runs the path-clearing helper and returns 0.
3. If the open set is empty, it returns 0.
4. **Selection.** The open set is walked in key order. For each cell, the stored record is looked up. The first cell that has a record is taken. A later cell replaces it only when its f (`+8`) is strictly less. Ties therefore go to the lexicographically smallest cell. The selected cell C is removed from the open set. Its record stays in the map, so a cell with a record that is not in the open set is closed.
5. If C equals the goal cell, the routine returns 0 without expanding it.

If no open cell had a record, the first open cell is removed and later steps would use a missing record. That case is not supplied.

### 4.2 Block scan around the node

1. The node point P is C × 65536 plus an offset. The offset is 32768 in x and y (binary64 `32768.0` at `0x302A88`, truncated). In z it is the truncation of binary64 (binary64(S `+0x88`) × `0.5` × `65536.0`) (`0x345DE8`, `0x2FD588`).
2. The minimum cell is the cell of (P − half extents + (0, 0, 655)). The 655 is the fixed (trunc) value of `0.01` (`0x3C23D70A`). The maximum cell is the cell of (P + half extents). The half extents use helper `0x02C460` (binary32 × `65536.0`, truncated).
3. The scan box runs from minimum − 1 to maximum + 1 on each axis, giving bounds Xlo..Xhi, Ylo..Yhi and Zlo..Zhi. The loops are x outer, y middle and z inner, each upward. Cells strictly inside the box on all three axes are not sampled.
4. **Block lookup.** A cell is looked up as follows:
   - If x or y is outside [0, `0x1000000`), or no zone is loaded (world helper `0x034D10` at x/256, y/256), the cell is the *solid default*, type 1.
   - Otherwise the zone column is entry ((y mod 256) × 256 + (x mod 256)) × 32 bytes from zone `+0xA8`. Below the column base (column `+0x10`), the cell is the solid default.
   - At or above base + count (column `+0x1C`), the cell is the *sea default* (type 2, flag `0x80`) when z ≤ 0, and the *air default* (type 0) otherwise.
   - Inside the column, the stored block is used, through column helper `0x02F730` with index z − base. A stored block of type 0 with z ≤ 0 and bit `0x40` clear is replaced by the sea default.

   The defaults are written at start-up by `0x2FABC0` (sea: bytes FF FF FF `0x82`), `0x2FAB60` (air: FF FF FF `0x00`) and `0x2FAB90` (solid: C8 C8 C8 `0x01`).
5. Each blocked (impassable) cell sets the flags below. "Mid" means strictly between the bounds on that axis. All other sampled positions set nothing.

| Flag | x | y | z |
|---|---|---|---|
| −x−y floor corner | Xlo | Ylo | Zlo |
| +x−y floor corner | Xhi | Ylo | Zlo |
| −x+y floor corner | Xlo | Yhi | Zlo |
| +x+y floor corner | Xhi | Yhi | Zlo |
| −x−y edge, +x−y edge, −x+y edge, +x+y edge | corner columns as above | | mid |
| floor (under the body) | mid | mid | Zlo |
| ceiling (above the body) | mid | mid | Zhi |
| −x floor edge, +x floor edge | Xlo / Xhi | mid | Zlo |
| −y floor edge, +y floor edge | mid | Ylo / Yhi | Zlo |
| −x, +x, −y, +y ceiling edges | as the floor edges | | Zhi |
| −x, +x, −y, +y side walls | as the floor edges | | mid |

Corners at Zhi are sampled but set no flag.

6. **Water indicator.** This is bit `0x02` of byte 3 of the world block containing (P.x, P.y, P.z − truncation of `−65.536`), that is, P.z + 65. The lookup is world helper `0x02F860`. It floors negative coordinates by subtracting one block before truncating division, and then uses `0x02F7E0`, which applies the same column and default rules with its own copies of the defaults.

### 4.3 Object scan around the node

1. The sector range is computed for x and y: (minimum − 1)/8 to (maximum + 1)/8, with cell values and truncating division. The x loop is outer, and sectors outside [0, `0x200000`) are skipped. Zones and lists are found as in section 3.
2. For every list entry, in list order:
   1. An object present in the ignore set is skipped entirely.
   2. **Object under the body.** This test applies to every object that is not ignored, whatever its kind or height. Using P, the S half extents and the object footprint as in section 3, the object counts as under the body when x and y overlap with the same inequalities, and also P.z + hz − 65536 ≥ obj.z and P.z − hz − 65536 < obj.z + oz.
   3. The side tests are skipped in any of these cases:
      - the object height (`+0x2C`) is less than `3.0` (`0x2FCD70`), and none of the ceiling flag or the four ceiling-edge flags is set;
      - the object kind is `1`, `2`, `8`, `7` or `6`;
      - the 12 bytes at list-entry `+0x0C` equal subject `+0x1478..+0x1480`.
   4. **Side tests.** Each test shifts the body by one block, 65536, on one axis only:
      - −x and +x shift x; −y and +y shift y.
      - The shifted axis uses the shifted position in both of its inequalities.
      - The other horizontal axis uses the unshifted overlap test.
      - The vertical test is P.z + hz ≥ obj.z and P.z − hz < obj.z + oz, through the comparison helpers `0x194690` (≥) and `0x194660` (<).

      A passing test sets the corresponding *object side* flag (−x, +x, −y or +y).

### 4.4 Neighbour expansion

The moves below are tried in this order. For each tried cell N, the step is as follows:

- **New.** If N has no record, a record is inserted with g = C.g + step cost, h = estimate(N, goal), f = g + h and parent = C. The move then counts as **taken**.
- **Improved.** If N has a record and is in the open set, and the new g is strictly less than the stored g, the record is replaced with the new g, the stored h, f = new g + stored h and parent = C. The move counts as **taken**.
- **Otherwise** (closed, or not cheaper), nothing changes, and the move is **not taken**.

"Floor" below means the floor flag from 4.2. "Water" means the water indicator. "Under" means any object under the body (4.3).

| Order | Move | Rejected if | Requires any of | Cost (new / improved) |
|---|---|---|---|---|
| 1 | x − 1 | −x side wall, or −x object side | floor, −x floor edge, −y side wall, +y side wall, −x−y edge, −x+y edge, water | 10 with floor, else 20 / same |
| 2 | x + 1 | +x side wall, or +x object side | floor, under, +x floor edge, −y side wall, +y side wall, +x−y edge, +x+y edge, water | 10 / 20 |
| 3 | y − 1 | −y side wall, or −y object side | floor, under, −y floor edge, −x side wall, +x side wall, −x−y edge, +x−y edge, water | **new: 10 with floor, else 30**; improved: 10 / 20 |
| 4 | y + 1 | +y side wall, or +y object side | floor, under, +y floor edge, −x side wall, +x side wall, −x+y edge, +x+y edge, water | 10 / 20 |
| 5 | x − 1, y − 1 | −x−y edge, or move 1 or 3 not taken | floor, under, −x−y floor corner, water | 14 with floor, else 24 |
| 6 | x + 1, y − 1 | +x−y edge, or move 2 or 3 not taken | floor, under, +x−y floor corner, water | 14 / 24 |
| 7 | x − 1, y + 1 | −x+y edge, or move 1 or 4 not taken | floor, under, −x+y floor corner, water | 14 / 24 |
| 8 | x + 1, y + 1 | +x+y edge, or move 2 or 4 not taken | floor, under, +x+y floor corner, water | 14 / 24 |
| 9 | z − 1 | floor, or under | (none) | 0 (g unchanged) |
| 10 | z + 1 | ceiling | any side wall (−x, +x, −y, +y), or water | 0 with floor, else 40 |

Notes on the table:

- Move 1 does not accept "under" as support, while moves 2 to 8 do. The asymmetry is in the original.
- The y − 1 new-record cost of 30 without a floor is also original. Its improved-record path uses 20.
- A diagonal depends on whether its two orthogonal moves were *taken* in this call, meaning newly inserted or improved. It does not depend on their passability alone.
- All costs and sums are wrapping signed 32-bit values. The existing-record comparison is signed.

The routine returns 1 after move 10 is handled, or after move 10 is skipped.

## 5. Path rebuild `0x1A7C90`

This routine takes the subject as its only stack argument; it uses no world. It does the following in order:

1. It computes A = cell of the start point and G = cell of the goal point.
2. It releases every path-list node and sets the count to 0.
3. The current cell K is the best record's cell, or G when there is no best record. K is inserted at the front of the path list.
4. A counter starts at 0. Then the following repeats:
   1. Stop if K equals A.
   2. Stop if the counter is greater than the record count `+0x1410`.
   3. Increment the counter, then look up K's record. Stop if there is none.
   4. Let K′ be the record's parent. Stop if K′ equals K.
   5. Insert K′ at the front of the list and set K to K′.

The list therefore runs from the start side (front) to the best cell (tail). The start cell is included when it is reached. Exceeding the list's length limit raises the runtime length error. The best record is the open or closed cell with the smallest h, with the earliest record winning ties, so the tail is the reached cell closest to the goal by the estimate. It is not necessarily the goal.

## 6. Direct-approach test `0x209480`

Arguments: the world in `this`, then the subject, a pointer to S's position, a pointer to T's position, and the reach r (binary32). The routine returns a byte and makes no stores outside its own frame.

Conversions use 64-bit difference → binary32 (x87 load and store, one rounding) × `1/65536` (`0x2FCD94`).

1. **Immediate test.** If r² > (dy² + dx²), where d = S − T converted, and S `+0x88` + r > |dz| (converted z difference), the routine returns **1**. Both comparisons are strict.
2. **Step count.** Let v = T − S horizontally, converted (third component 0). If (v.y² + v.x²) + 0 > `2500.0` (`0x302ABC`), v is rescaled to length `50.0` (`0x2FCD7C`) using 1 / binary32(binary64 square root). The step count N is the truncation of (binary32 of the binary64 square root of ((v.y² + v.x²) + `0.5625`)) + `1.0`. If N ≤ 0, the routine goes straight to step 4.
3. **Simulated walk.** A probe point P starts at S's position. For each of the N steps:
   1. If (dy² + dx²) ≤ r², with d = P − T converted, and S `+0x88` + r > |dz|, the routine returns **1**. The horizontal comparison here is non-strict.
   2. The direction u is T − P horizontally, converted. If its squared length is positive, u is normalised with the reciprocal of the rounded binary64 square root. Its third component is set to `−0.71` (`0xBF35C28F`).
   3. For axis k = x, then y, then z:
      1. The step s = fixed (trunc) of u[k] is added to P[k].
      2. **World test**: collision helper `0x19F0A0` (section 7) at P with S's size triple `+0x80`.
      3. If the world test hits and k is not z, the step is **world-blocked**.
      4. Otherwise an **object test** runs. It scans the 3 × 3 sectors around P, with sector = (P / 65536 truncated) / 8 truncated, x loop outer. Objects of kind `7`, `6` or `9` are skipped. Objects in the ignore set are skipped. Kinds `1`, `8`, `2`, `3` and `5` require byte `+0x30` nonzero.
         - The object must lie ahead along k: the truncation to int64 of binary32(P[k] − obj[k], as a 64-bit integer converted to binary32 without scaling) × u[k] must be negative.
         - The object must also overlap the body at P under the section 3 inequalities.
         - Such an object makes the step **object-blocked**.
         - For k = z with a world hit, the step is blocked whatever the objects are.
      5. A blocked step is undone (P[k] −= s), except in the step-up case below.
      6. **Step-up.** This is allowed only when k is x or y, the step is world-blocked (not object-blocked), and bit `0x0100` of S's 16-bit `+0x7E` is clear. The routine samples every cell (no interior skip) from cell(P + (0, 0, f) − half extents) to cell(P + (0, 0, f) + half extents), where f is the fixed (trunc) value of `1.01` (`0x3F8147AE`). x is the outer loop, then y, then z, using the section 4.2 block lookup with its own defaults (start-up routines `0x2FB050`, `0x2FAFF0` and `0x2FB020`).
         - If any sampled cell is impassable, the step is undone.
         - Otherwise the step is kept and P.z is replaced. Let m be the 32-bit block index of P.z − hz: one block is subtracted before division when the value is negative, then signed truncating division by 65536 is applied. The new P.z is m × 65536 − (truncation of binary64 `−66191.36`, `0x31E220`) + hz + (truncation of binary32 `66191.36`, `0x31E208`). That equals m × 65536 + 132382 + hz. Applying the 1.01 lift twice is original behaviour.
      7. A blocked z step (falling onto ground or an object) is undone.
4. After all N steps, the routine returns **1** when ((P − S).y² + (P − S).x²) + (P − S).z² > `625.0` (`0x31E200`), with S its starting position, and **0** otherwise.

The far route treats a result of 1, with S `+0x5C` bit `0x20` clear, as "approach directly": it clears the path and returns. A walk that ends more than 25 units from its start therefore also counts as success, even when T was not reached.

## 7. World collision test `0x19F0A0`

Arguments: the world in `this`, then a point P, a size triple, and a flag. The half sizes are fixed (trunc) of (size × 0.5).

1. The cell bounds are (P − half) and (P + half), each signed-divided by 65536 with **truncation**. This is not the floor-adjusted cell rule.
2. Every cell from the minimum to the maximum is sampled except cells strictly interior on all three axes. x is the outer loop, then y, then z.
3. The block lookup is as in 4.2, with the defaults written by `0x2FAB30` (sea), `0x2FAAD0` (air) and `0x2FAB00` (solid).
4. The first impassable cell returns **1**.
5. When the flag is 0, the case used here, the routine returns **0** if no cell blocks. The object branch taken for a nonzero flag was not audited.

## 8. Far-route use and tick cadence

These rules combine the held contract with this one.

- The path request runs only when the path list is empty, together with the held contract's T `+0x5C` condition. It resets the record map, the open set, the start record and the ignore set.
- On every far-route invocation that reaches path maintenance with a non-empty record map, up to 10 iterations of expansion followed by rebuild run. The held contract defines the early exits: list length above 50, or the tail cell within (S `+0x80`)² squared cell distance of the goal cell.
- The search is discarded, through the path-clearing helper, in these cases:
  - the far-route give-up reset;
  - a positive direct-approach result;
  - every near-route invocation;
  - the cap exit in 4.1;
  - the follower's arrival test (section 9).

  Because the clearing empties the path list, the next far route issues a new request.
- Each search call expands one cell. A long search therefore spans several invocations. The open-set cap (500) and the record cap (20000) end it.

## 9. Per-tick path follower (partial)

World routine `0x1A7EB0` (`this` = world; arguments: subject, step) is called from actor update `0x20C510` (call site `0x211C30`). It runs only when the path list is non-empty and S `+0x12C` ≤ 0. Just before the call, the update calls `0x02F160` on S `+0x10` with arguments 4 and 0; that helper was not audited. When the follower returns nonzero, the update calls the path rebuild (`0x211DE0`). The update also adds the step to the stall timer `+0x145C` at `0x21CCF2`.

Confirmed from the listing:

1. The movement triplet `+0x40/+0x44/+0x48` is set to zero.
2. Let F be S's position plus (0, 0, fixed (trunc) of (`0.5` − S `+0x88` × `0.5`)). If (+0x1458)² ≥ the squared distance between the cell of F and the goal cell (the squared distance is a wrapping 32-bit sum converted to binary32), the path-clearing helper runs and the routine returns 0.
3. If the path list is empty, it returns 0.
4. If the stall timer is greater than `3000`:
   - the start point is set to F, using `0x02C460` and `0x02C800`;
   - the path request runs, which also resets the stall timer;
   - the routine returns 0, because the list is now empty.
5. The nearest path node is found:
   - The reference point is S's position with x and y unchanged and z reduced by the truncation of binary32 (S `+0x88` × `0.5` × `65536.0`).
   - For each list node in order, the squared cell distance from the cell of the reference point is computed. Later nodes win ties (≤).
   - If the minimum is 0, the stall timer is set to 0.
6. Final movement: the steering vector is scaled by `20.0` (`0x2FCD74`) when the nearest squared distance is below 4, and by `40.0` (`0x2FCE40`) otherwise. It is stored to the movement triplet. The triplet `+0x4C/+0x50/+0x54` is then zeroed, and the routine returns 1. Two other paths return early, and their return values were not confirmed: a nearly zero steering vector (squared length at most `0.001`, `0x2FD870`), and the line-of-sight branch when S `+0x124` bit `0x01` is clear.

The following were seen only as leads and are **not specified**: the look-ahead checks through world helper `0x19F850`; re-rooting the search at the current node, including pruning up to 70 open cells; the door-opening store on objects of kind 1 and 2 (clearing object byte `+0x30` and dword `+0x34`); the smoothing loops on the velocity triplet (`0.05`, `0.01`, `0.001`; scales `4.0`, `6.0`, `12.0`, `8.0`; vertical clamps `1.0`, `−2.0`, `3.0`); and the `+0x114` flag-bit update. They need a dedicated contract. The client listing also stops partway through this routine (see section 11).

## 10. Random draws

The static call closure of `0x1A9F90`, `0x1AAAB0`, `0x1A7C90` and `0x02EF10` contains 66 functions: the container helpers, the arithmetic runtime, block and zone lookup, and memory allocation. It contains no call to `rand` or `srand`. Its imports are allocation and free, `memmove`, and the container-length, out-of-range and bad-allocation error raisers. The only unresolved transfer is the stack-cookie failure trap. In the four routines, each indirect call resolves to one of these imports.

`0x209480` and `0x19F0A0` call only the runtime truncation and division helpers, the square-root import, the cell helper and column helper `0x02F730`. They make no direct or transitive draws.

**The far-route helpers therefore consume no random numbers.** The held contract's draw count and order stand.

## 11. Client/server agreement

Server counterparts were found from the server first-child routine `0x002F40` at the call sites corresponding to the client's:

| Client | Server | Comparison |
|---|---|---|
| `0x02EF10` path clear | `0x005330` | 51 server lines = 47 client lines + 4 decoded skipped-byte instructions, identical |
| `0x209480` direct approach | `0x12EF00` | 1153 instructions, identical |
| `0x1A9F90` request | `0x0DD2E0` | 803 instructions, identical |
| `0x1AAAB0` expansion | `0x0DDE90` | 2582 instructions, identical |
| `0x1A7C90` rebuild | `0x0DAFE0` | 160 = 156 + 4 decoded skipped-byte instructions, identical |
| `0x19F0A0` collision | `0x0D4F90` | 580 instructions, identical |
| `0x1A7EB0` follower | `0x0DB200` | not established: the client listing skips bytes after a deallocation that the server decodes |

Instructions were compared with absolute addresses set aside. The paired data operands of the expansion step, the request, the direct-approach test and the rebuild were then compared byte by byte, and every constant has the same value. Examples: `0x2FD588` ↔ `0x1737F0`, `0x302A88` ↔ `0x1737E8`, `0x31DBB0` ↔ `0x173800`, `0x2FCD7C` ↔ `0x158828`, `0x302ABC` ↔ `0x173CBC`, `0x31E1E4` ↔ `0x173C90`, `0x31E200` ↔ `0x173CB0`, `0x31E208` ↔ `0x173CC4` and `0x31E220` ↔ `0x173CE0`. Differences appear only in bytes beyond a 4-byte binary32 operand, and the routines never read those bytes. The server's default-block initialisers write the same bytes as the client's: `0x157200`, `0x1571A0` and `0x1571D0` for the expansion step, and `0x157560`, `0x157500` and `0x157530` for the direct-approach test. Client and server therefore agree on every routine in this contract except the follower.

## 12. Unresolved

- The follower `0x1A7EB0`: everything section 9 lists as unspecified, its early-return values, and client/server agreement.
- The meaning of subject `+0x1478..+0x1480` and of the 12 bytes at sector-list entry `+0x0C`. They are compared only.
- The object branch of `0x19F0A0` for a nonzero flag, and helpers `0x19F850`, `0x02F160` and `0x034A90` beyond what is stated.
- Behaviour when no open cell has a record (4.1), with overflow, with negative coordinates outside the stated bounds, and with invalid containers.
- The defaults used by the collision test `0x19F0A0` on the server, which were not paired byte by byte. Its instruction sequence is identical.
- The gameplay meaning of object kinds, block types other than 0, 1 and 2, the water bit and S `+0x7E` bit `0x0100`.
