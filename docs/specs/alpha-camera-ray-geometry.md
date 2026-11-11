Status: FINAL / STABLE / HELD

# Alpha client ray query used by the camera follow/collision caller

This is an independent, code-free behavioral contract for the client ray query at RVA 0x1A35D0 (image VA 0x005A35D0), written for the two call shapes that reach it from the late camera range 0x09C2E6-0x09CBE3 of the client frame routine RVA 0x088EE0. It covers the complete body of the query (0x1A35D0-0x1A470A) and the helpers it reaches. It does not claim coverage of the producers of the world tables, columns, voxel records or box records that the query reads.

## Image and evidence

The supported original is `binary/Cube.exe` under the evidence root `disassembled alpha cube world`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (re-verified for this contract), preferred image base 0x00400000. Section layout read from the image header: `.text` RVA 0x001000, `.rdata` RVA 0x2FC000, `.data` RVA 0x365000 with virtual size 0x92F8 and only 0x6000 bytes of file-backed data. Addresses are RVAs unless marked VA.

Evidence used: the instruction listing `decompiled/client/assembly.txt` (called `A` below) for 0x1A35D0-0x1A4708, for the helpers 0x02F730, 0x034A90, 0x034D10, 0x28D946, 0x28D9E0, and for the seven call sites listed later; `decompiled/client/functions/005a35d0.c` as a lead only; and the image itself for every constant and for spot checks of instruction bytes (routine entry, voxel classification at 0x1A3774, the second-option test at 0x1A3A93, the per-axis scan start at 0x1A38B0, the step/continue block at 0x1A3A1C, the record filter at 0x1A3C63-0x1A3D07, the epilogue at 0x1A46F2, and the entries of 0x02F730, 0x034D10 and 0x28D946). Not every instruction byte of the routine was compared with the image; the listing was otherwise relied on. No separate per-function assembly file exists for this routine. No runtime observation, reconstruction material, earlier research prose or replacement implementation supplied any fact stated here; the earlier caller contract was used only to locate the call sites, which were then re-read in `A`.

## Interface

The routine is a member-style call: the receiver arrives in ECX and the callee removes 20 bytes of stack arguments (return with operand 0x14 at 0x1A4708). The receiver is the world object, which for the late camera calls is the world embedded in the client at client+0x2E4. In stack order the five arguments are:

1. A pointer to the start point: three signed 64-bit fixed coordinates (x, y, z), 65,536 raw units per coordinate unit. The routine only reads it.
2. A pointer to the direction: three binary32 components (x, y, z). The routine only reads it and never normalizes it.
3. The maximum, one binary32 passed by value.
4. First option, tested as the low byte of its stack slot.
5. Second option, tested as the low byte of its stack slot (zero versus nonzero).

The result is returned on the x87 stack, loaded from a binary32 local (0x1A46F5), so it carries exactly a binary32 value. Both camera call sites store it straight back to binary32.

The result is a ray parameter: the number by which the direction is multiplied to reach a point from the start, in coordinate units per unit of direction. It equals a distance in coordinate units only when the supplied direction has unit length. The corner-sample call passes the caller's un-normalized backward direction; the final call passes a direction its caller has just normalized.

## Meaning of the two options

First option. The routine classifies each visited voxel as blocking (1) or non-blocking (0) and keeps stepping only while that classification, as a byte, equals the first option byte. With value 0 the walk passes through non-blocking voxels and stops at the first blocking one. With value 1, which is what both camera calls pass, the walk passes through blocking voxels and stops at the first non-blocking one. Any other byte value can never equal the classification, so the walk ends before its first step.

Second option. When nonzero, a second phase runs after the voxel walk and may lower the result further by testing the ray against axis-aligned box records stored per map cell (described below). When zero, the voxel-walk result is returned unchanged. Both camera calls pass 1.

Values passed by every direct call site found in `A` (seven sites, located by searching the listing for direct calls to VA 0x005A35D0; indirect calls, if any exist, are not excluded):

| Call site RVA | Maximum | First option | Second option |
| --- | --- | --- | --- |
| 0x09848E (frame routine 0x088EE0, earlier range) | binary32 60 (bits 0x42700000) | 0 | 1 |
| 0x09C997 (camera corner sample) | caller's current distance | 1 | 1 |
| 0x09CB74 (camera final correction) | binary32 100 (bits 0x42C80000) | 1 | 1 |
| 0x0B4743 (routine 0x0AC260) | binary32 100 | 0 | 1 |
| 0x19F02F (routine 0x19EE90) | computed | 0 | forwarded from that routine's own third stack argument |
| 0x215FD8 | binary32 200 (bits 0x43480000) | 0 | 1 |
| 0x222293 (routine 0x20C510) | computed | 0 | 1 |

Only the two camera calls pass a nonzero first option. The purposes of the other callers were not examined.

## Phase one: the voxel walk

### Starting cell

Each start coordinate is divided by 65,536 as a signed 64-bit division truncating toward zero (helper 0x28D9E0) and the low 32 bits of the quotient are kept. If the raw coordinate is negative (strictly below zero as a signed 64-bit value), one is then subtracted from that 32-bit value. A negative coordinate that is an exact multiple of 65,536 therefore maps one cell below its plain quotient; this is not a mathematical floor. This is the same result as the fixed-coordinate adapter used by the caller's own voxel test. Source: `A` 0x1A35E6-0x1A367E.

The running result, called t here, starts as binary32 positive zero, and a signed 32-bit step counter starts at zero.

### Entry test

If the maximum is not strictly greater than zero (zero, negative, or not a number), no voxel is examined and no step is taken. Source: 0x1A3681-0x1A3696.

### Per-iteration order

Each iteration does the following, in this order.

1. Classify the current cell (next subsection). If the classification differs from the first option byte, leave the walk.
2. If the step counter is strictly greater than 500 (signed), leave the walk. Because the counter counts completed steps, at most 501 steps can be taken.
3. Increment the step counter.
4. Form the current point: each direction component is multiplied by t in binary32; each product is multiplied by binary32 65,536 as a separate binary32 operation; each result is converted to a signed 64-bit integer by truncation toward zero (helper 0x28D946); each integer is added to the corresponding raw start coordinate with 64-bit wrap.
5. Scan the axes in the order x, y, z to choose a step length and a step axis (subsection below).
6. Move the cell: if the direction component of the chosen axis is strictly greater than zero, add one to that axis's cell index; otherwise (zero, negative, or not a number) subtract one.
7. Replace t by the binary32 sum of t and the chosen step length.
8. Continue only while the maximum is strictly greater than the new t; otherwise leave the walk.

The cell indices are advanced only by these unit moves; they are never recomputed from the current point. The current point is recomputed from t on every iteration and is used only to measure the distance to the next cell face. Source: 0x1A36A0-0x1A3A5D.

### Step length and axis

The best length starts as binary32 10 and the best axis starts as x. For each axis in order:

- The direction component is squared in binary32. If the binary32 threshold with bits 0x358637BD (approximately one millionth) is strictly greater than that square, the axis is skipped. A not-a-number component is not skipped by this test.
- Otherwise a face coordinate is formed as a signed 64-bit value: the axis's 32-bit cell index, plus one (32-bit wrap) when the direction component is strictly greater than zero and unchanged otherwise, sign-extended to 64 bits and shifted left by 16.
- The current point's raw coordinate on that axis is subtracted from the face coordinate (64-bit wrap).
- That difference is converted to binary32 (a single rounding of the exact integer), then divided by the direction component in binary32.
- The quotient is converted to a signed 64-bit integer by truncation toward zero, converted back to binary32, and multiplied by the binary32 with bits 0x37800000 (exactly 1/65,536). This is the axis's candidate length. The truncation means candidates are whole numbers of raw units divided by 65,536.
- The candidate replaces the best length, and the axis becomes the best axis, only when the best length is strictly greater than the candidate. On a tie the earlier axis is kept.

If every axis is skipped, the step is 10 along the x axis, and the x cell index is decreased by one unless the x direction component is strictly positive. Source: 0x1A3826-0x1A3A36.

The routine applies no lower bound to a candidate, so nothing in the code itself forces each step, or t, to be nonnegative. In exact arithmetic the candidates are nonnegative while the tracked cell contains the current point; whether rounding and the truncations can ever make the tracked cell and the current point disagree enough to produce a negative step was not proved either way.

### After the walk

On every exit from the walk, and also when the walk was skipped by the entry test, t is replaced by the maximum when t is strictly greater than the maximum. Consequences: a negative maximum yields the maximum itself; a zero maximum yields zero; a not-a-number maximum yields positive zero. Source: 0x1A3A62-0x1A3A8B.

## Voxel fetch and classification

For a cell with integer indices (cx, cy, cz):

1. If cx or cy is negative, or is 0x01000000 or more, the cell is treated as missing.
2. Otherwise the chunk indices are cx and cy each divided by 256 (signed, toward zero; here both are nonnegative). Helper 0x034D10 is called on the receiver with the x chunk index first and the y chunk index second. That helper requires each index to be in 0..0xFFFF, divides each by 64 to obtain region indices (each must be in 0..0x3FF; helper 0x034A90), reads a region pointer from the receiver at offset 0xBC plus four times (x region index times 1,024 plus y region index), and, if that pointer is nonnull, reads the chunk pointer from the region at offset 0x10018 plus four times (x chunk index modulo 64, times 64, plus y chunk index modulo 64). A null region or null chunk pointer means missing. In this image 0x034D10 and 0x034A90 only read; they neither allocate nor load anything.
3. The column address is the pointer stored at chunk+0xA8 plus 32 times (cy modulo 256, times 256, plus cx modulo 256). Columns are 32 bytes. The routine tests the computed address, not the stored pointer, for null; a null stored pointer is therefore not safely handled except for column index zero.
4. The column's signed 32-bit base height is at column+0x10 and its signed 32-bit record count at column+0x1C. If cz is below the base, the cell is treated as missing.
5. If cz is at or above base plus count (32-bit sum), the cell uses a static record: the one at VA 0x0076B7CC when cz is zero or negative, the one at VA 0x0076B7D0 when cz is positive.
6. Otherwise the record is the four-byte entry number (cz minus base) in the array whose pointer is at column+0x18 (helper 0x02F730; with an index inside the column it performs only that address computation and writes nothing). If that record's byte 3 has its low five bits all zero, and cz is zero or negative, and bit 0x40 of byte 3 is clear, the static record at VA 0x0076B7CC is used instead.
7. A missing cell uses the static record at VA 0x0076B7D4.

Classification uses only byte 3 of the chosen record: take its low five bits; the cell is non-blocking when that value is 0 or 2 and blocking for every other value. Source: 0x1A36A0-0x1A378D; helpers in `A` at 0x02F730, 0x034A90, 0x034D10.

The three static records at VA 0x0076B7CC, 0x0076B7D0 and 0x0076B7D4 lie in the part of `.data` that has no file-backed bytes, so they are zero when the image is loaded. A search of the listing found no instruction that stores to any of their addresses by absolute address; the only references are address loads inside routines 0x19A0E0, 0x19AEB0, 0x19C890, 0x19CBF0, 0x19F0A0, 0x19F850 and this routine. Those other routines were not audited, so a store through a pointer they hand out is not excluded. While the static records remain zero, every missing cell, every cell below a column, and every cell above a column classifies as non-blocking, and the substitution in step 6 never changes a classification.

These static records are different addresses from the ones used by the voxel lookup the camera caller applies before it calls this routine. That the two lookups always agree for the same cell was not proved.

## Phase two: box records (second option nonzero)

### Cell range

Two map-cell indices are formed for the start: the start x and y raw coordinates are each divided by 65,536 (signed, toward zero, low 32 bits kept, with no adjustment for negative values), then each divided by 256 (signed, toward zero). Two more are formed from the walk's final x and y cell indices (which do include the negative adjustment and every unit move, including the move made in the last iteration), each divided by 256 the same way. On each axis the smaller of the two becomes the lower bound and the larger the upper bound. Every index pair inside this inclusive rectangle is visited, x in the outer progression and y in the inner one, both ascending. Source: 0x1A3A9D-0x1A3B92 and 0x1A46BF-0x1A46EC.

For each pair, both indices must be in 0..0xFFFF, and the region and chunk pointers are fetched from the same two tables and with the same index arithmetic as in the voxel fetch above (inlined here rather than called). A pair with an out-of-range index, a null region or a null chunk contributes nothing.

### Records

In a chunk, the pointer at chunk+0x0C is the first record and the pointer at chunk+0x10 is the end; records are 0x188 bytes apart and are visited in ascending address order. The end pointer is read once per chunk. A record takes part only when its 32-bit value at +0x00 is 3, 4, 5 or 8 and its byte at +0x30 is nonzero. What those kind values and that byte represent was not established.

Fields read from a participating record:

- +0x08, +0x10, +0x18: three signed 64-bit raw coordinates, called Px, Py, Pz here.
- +0x20: a signed 32-bit value of which only the parity matters.
- +0x24, +0x28, +0x2C: three binary32 extents. When the value at +0x20 is odd (either sign), the first two are exchanged before use. After that possible exchange they are called Wx, Wy, Wz here.

Derived raw quantities, each produced by separate binary32 operations followed by truncation toward zero to signed 64 bits: Hx is Wx multiplied by binary32 0.5 and then by binary32 65,536; Hy is Wy treated the same way; Hz is Wz multiplied by binary32 65,536 only, with no half factor. The box therefore spans Px minus Hx to Px plus Hx, Py minus Hy to Py plus Hy, and Pz to Pz plus Hz.

### Face tests

Only the four vertical side faces are tested; the bottom and top faces are never tested.

If the direction x component is not equal to zero (a not-a-number component counts as not equal), two candidates are examined, in this order: the face at Px minus Hx, then the face at Px plus Hx. For the first, the raw value is Px minus Hx, then minus the raw start x; for the second it is Px minus the raw start x, then plus Hx (64-bit wrap throughout). Each raw value is converted to binary32 with one rounding, multiplied by binary32 1/65,536, then divided by the direction x component, giving a candidate parameter. A candidate is considered only when it is greater than or equal to zero and the current result is strictly greater than it. For a considered candidate, a hit point is formed exactly as the walk forms its current point (direction times candidate in binary32, times 65,536 in binary32, truncated to 64 bits, added to the raw start). The candidate becomes the new result when all of the following hold as signed 64-bit comparisons: hit y is at or above Py minus Hy; hit z is at or above Pz; hit y is strictly below Py plus Hy; hit z is strictly below Pz plus Hz.

Then, if the direction y component is not equal to zero, the same is done for the face at Py minus Hy and then the face at Py plus Hy, dividing by the direction y component, and testing hit x against Px minus Hx (inclusive) and Px plus Hx (exclusive) and hit z against Pz (inclusive) and Pz plus Hz (exclusive). If the y component equals zero the record is finished after its x faces.

The running result is shared across all faces, records and cells, and a candidate replaces it only when strictly smaller. Since each candidate's own admission does not otherwise depend on the running result, the final value is the smallest of the phase-one result and all admitted candidates; visitation order does not affect it. Phase two can only lower the result, and only to values that are greater than or equal to zero. Source: 0x1A3C4F-0x1A46B0.

## What the result means for the two camera call shapes

Corner-sample shape (fixed start at a sample the caller found blocked, un-normalized backward direction, maximum equal to the caller's current distance, options 1 and 1):

- For a maximum that is not strictly positive, the result is the maximum itself when it is zero or negative.
- Otherwise the result never exceeds the maximum (final replacement rule) in the ordered case.
- Phase one advances through consecutive blocking cells and ends at the first cell the routine classifies as non-blocking; the phase-one value is then the parameter accumulated on entering that cell, limited to the maximum. It is not an exact intersection parameter: every step length has passed through the truncation and rounding described above, and the direction and size of the resulting error were not bounded here.
- If the routine's own classification of the starting cell is non-blocking, phase one contributes zero.
- If every cell stays blocking, the result of phase one is the maximum, or the parameter accumulated after 501 steps if the cap is met first (still limited to the maximum).
- Missing data, a cell outside the horizontal range, a cell below its column or above its column ends the blocking run exactly like an empty cell, as long as the static records hold zero.
- Phase two may then lower the value to the nearest admitted side-face parameter of a participating box record in the visited map cells.

Final-correction shape (start at the followed creature's position, normalized direction, maximum 100, options 1 and 1): identical rules with maximum 100. In particular, if the creature's own cell is classified non-blocking, phase one contributes zero and the result is zero, because phase two cannot raise it and admits no candidate below zero. The result is at most 100. A negative result is not excluded by the code for the reason given under "Step length and axis"; no case producing one was demonstrated.

There is no distinct no-hit or missing-data sentinel value in either shape: the routine always returns a parameter of the kind described above.

## Conversions and helper behavior

- Helper 0x28D9E0 is a signed 64-bit division that truncates toward zero.
- Helper 0x28D946 converts the value on the x87 stack to a signed 64-bit integer truncating toward zero for in-range inputs, independent of the current rounding direction; the routine always feeds it a value already stored as binary32. Out-of-range and not-a-number inputs are outside the relations stated here.
- Signed 64-bit integers are converted to binary32 by an x87 integer load followed by a binary32 store, which is one rounding of the exact integer under the current rounding direction. The default rounding mode is assumed; the control word in force at the call was not traced.
- All multiplications, divisions and additions on real values in this routine are individual binary32 operations. No binary64 arithmetic occurs in the routine.
- Comparisons on real values are ordered comparisons whose unordered outcomes are as stated where they matter; other not-a-number and infinite cases are outside the ordinary domain of this contract.

## Constants

| Image VA | Bytes in image order | Bits | Value and use |
| --- | --- | --- | --- |
| 0x006FCE7C | 00 00 80 47 | 0x47800000 | binary32 65,536; scaling before truncation to raw units |
| 0x006FCD94 | 00 00 80 37 | 0x37800000 | binary32 1/65,536; scaling raw differences to coordinate units |
| 0x00745E74 | 00 00 20 41 | 0x41200000 | binary32 10; initial best step length |
| 0x0071DAF4 | BD 37 86 35 | 0x358637BD | binary32 approximately 0.000001; squared-component skip threshold |
| 0x00745DA0 | 00 00 00 3F | 0x3F000000 | binary32 0.5; half-extent factor in phase two |

Immediate values in the instruction stream: divisor 65,536 (0x10000) for the fixed-to-cell divisions; step-counter limit 500 (0x1F4) at 0x1A379C; horizontal cell bound 0x01000000; chunk index bound 0x10000; region index bound 0x400; region table offset 0xBC; chunk table offset 0x10018; column array offset 0xA8; column stride 32; column fields 0x10, 0x18, 0x1C; classification mask 0x1F and excluded values 0 and 2; substitution bit 0x40; record list offsets 0x0C and 0x10; record stride 0x188; record kinds 3, 4, 5, 8; record offsets 0x08, 0x10, 0x18, 0x20, 0x24, 0x28, 0x2C, 0x30; static record addresses VA 0x0076B7CC, 0x0076B7D0, 0x0076B7D4. Zero is produced by register clearing, not loaded from a constant.

## State, nested calls and random numbers

The routine writes only its own stack frame. It does not write through the start or direction pointers, to the receiver, to any chunk, column, voxel record, box record or static record, and it takes no lock. Its nested calls are 0x28D9E0, 0x28D946, 0x034D10 (which calls 0x034A90), 0x02F730 and the stack-protection check at 0x28D788 before returning; on the paths reached from here none of them writes program state. Helper 0x02F730 does contain branches that write other static storage, but they are taken only for an index that is negative or not below the column count, which the routine's preceding range tests exclude.

No random-number or seed routine is called, directly or through these helpers.

Because nothing is locked, the routine depends on the caller for any exclusion against concurrent changes to the tables it reads; that was not examined.

## Unresolved

- Whether a step length or the phase-one result can be negative under rounding; the code has no lower clamp.
- Whether the three static records are ever written at run time, and therefore whether missing or out-of-column cells can ever classify as blocking.
- Whether this routine's inlined voxel fetch always classifies a cell the same way as the caller's separate lookup.
- The meaning of box-record kinds 3, 4, 5 and 8, of the byte at record +0x30, of the parity field at +0x20, and of the rest of the 0x188-byte record; who creates and updates the record lists.
- The meaning of the voxel type values in the low five bits of record byte 3 beyond the two non-blocking values, and of bit 0x40.
- Producers, lifetime and concurrency rules of the region, chunk and column tables; behavior with a null column-array pointer.
- The floating-point control state at the call, and behavior for infinite, not-a-number or conversion-out-of-range values beyond the cases stated.
- Purposes of the five non-camera callers and the existence of any indirect callers.
- Byte-for-byte agreement of the whole listing with the image beyond the spot checks named above.
