Status: FINAL / STABLE / HELD. Full author readback completed.

# Creature hostility test and the line-of-sight test of the melee strike loop

This independently authored, code-free contract specifies the two-creature hostility test used by the melee strike loop (client RVA `0x1988A0`, server RVA `0x0D18C0`) and the geometric test that precedes it at that call site (client `0x19EE90`, server `0x0D4D80`). The held contracts `alpha-strike-call-sites.md` and `alpha-attack-routine.md` were used only to locate the two routines and their call site; every statement below was re-established from the original images. The Infinite-Cubes reconstruction was consulted for leads on field meaning only; every statement taken from it is marked as a reconstruction observation and is not a native fact.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`; every address below is an RVA. Pairs are written client / server. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method

- Instructions were read from `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt` (the per-function listing files for these routines are absent from `assembly-functions`). The decompiler output was not relied on.
- Data constants were read from the image bytes through the section table.
- Callers were found by scanning every section of each image for relative calls and jumps (opcodes `E8`, `E9` and `0F 80`–`0F 8F`) whose destination is the routine, and for any 32-bit absolute occurrence of the routine's address. Neither image contains an absolute occurrence of the hostility test, its nested predicate or the geometric test, so none of them is reached through a pointer table in the images.
- Draw statements rest on a transitive walk of `decompiled/<image>/calls.csv` from each routine, checked against the instruction listing for direct calls.
- Client and server versions were compared instruction by instruction after masking absolute addresses (section 7).

Terms: a **creature** is the object the world's creature container holds; **position** is its three signed 64-bit coordinates at `+0x10` (fixed point, 65536 units per world unit); **health** is its binary32 `+0x16C`. **Striker** and **target** are as in the held strike contract.

## 2. The hostility test (`0x1988A0` / `0x0D18C0`)

### 2.1 Interface

- Two 32-bit stack arguments, both creature pointers: first the **source** (the striker at the strike site), then the **other** creature. The routine removes its own 8 bytes of arguments on return.
- The value is returned in the low byte of the accumulator only: 1 means hostile ("passes"), 0 means not hostile. The upper 24 bits are not defined on every path; every caller tests only the low byte.
- Callers load the counter register (the world-update callers place the world there), but the routine never reads it. It reads no world or global state, makes no lookup, performs no floating-point work, makes no draws and writes no memory (only its own stack and registers).

### 2.2 Fields read

On each creature, at most:

| Offset | Width | Use |
| --- | --- | --- |
| `+0x60` | 8 bits | **Role byte.** Compared for equality with 6 and with 1 only (signedness irrelevant). |
| `+0x124` | 8 bits (low byte of a 16-bit word) | **Toggle bit 5**: tested with mask `0x20`. |
| `+0x7E` | 16 bits, zero-extended | Read by the nested predicate (2.3): mask `0x1A00`. |
| `+0x64` | 32 bits | Read by the nested predicate (2.3): equality with a fixed list. |

Health, keys, owner or pet links, target ids, the struck set and any player-versus-player state are not read.

### 2.3 Nested predicate (`0x044680` / `0x00F710`), fully audited

Called with one creature in the counter register; no stack arguments; returns in the low byte. It makes no calls, no draws and no writes. Call it **E** (exempt) for that creature:

1. When the 16-bit value at `+0x7E` has any of bits 9, 11 or 12 set (mask `0x1A00`: values `0x0200`, `0x0800`, `0x1000`), E is 0.
2. Otherwise E is 1 exactly when the 32-bit value at `+0x64` equals one of these 32 values, and 0 for every other value (including negative values and values above `0x93`):
   `0x13`, `0x14`, `0x16`, `0x17`, `0x19`, `0x1A`, `0x1B`, `0x1E`, `0x1F`, `0x20`, `0x21`, `0x22`, `0x23`, `0x35`, `0x37`, `0x38`, `0x39`, `0x3A`, `0x43`, `0x44`, `0x4A`, `0x57`, `0x59`, `0x5C`, `0x5D`, `0x62`, `0x64`, `0x6A`, `0x6B`, `0x91`, `0x92`, `0x93`
   (decimal 19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146, 147). The listing tests them in a different order; the order has no effect since the test is pure.

### 2.4 Decision

Let S be the source, O the other, r(x) the role byte and t(x) toggle bit 5 of `+0x124`. The relation is evaluated in this order and the first rule that applies decides:

1. **Same object.** When S and O are the same pointer, the result is 0. This is a pointer comparison, made before any field is read; two distinct objects with equal fields or equal keys are not treated as the same.
2. **Role 6.** When r(S) = 6 or r(O) = 6, the result is 1. (S is read first; when r(S) is 6, O's role is not read.)
3. **Exactly one role 1.** When exactly one of r(S), r(O) equals 1, the result is 1.
4. **Both role 1, differing exemption.** When both equal 1 and E(S) differs from E(O), the result is 1.
5. **Toggle bit.** When t(S) or t(O) is set, the result is 1.
6. Otherwise the result is 0.

Consequences, all read directly from the relation:

- The relation is symmetric in S and O.
- Every role value other than 1 and 6 (0, 2–5, 7–255) behaves identically: such creatures are hostile to each other only through rule 5, and hostile to a role-1 creature always (rule 3), and to a role-6 creature always (rule 2).
- Two role-1 creatures are hostile when exactly one of them is exempt (rule 4), otherwise only through rule 5.
- Dead creatures are not distinguished: health is not read, so a creature with health at or below zero is treated like a living one. Callers filter health themselves.
- A creature tested against itself always yields 0, even when its role is 6 or its toggle bit is set.
- When both creatures have role 1 and equal exemption, E is evaluated four times (twice per creature); when they differ, twice. The repetition has no effect.

### 2.5 Meaning of the fields, as far as traced producers show

Traced producers of the role byte `+0x60`, in the server image only (client producers were not traced):

- `0x025A19` (inside the routine at `0x0254A0`): an initialiser stores role 0, health from a computed value, and `+0x64` = `0x2C`.
- `0x134612` (world update): a newly constructed creature receives role 5, and its `+0x198`/`+0x19C` receive the 64-bit key of the creature being processed (its creator).
- `0x1348B2` (world update): for a role-5 creature, the creator key at `+0x198` is looked up. When the lookup finds a creature whose 64-bit `+0x11C8` differs from the role-5 creature's own key, the role-5 creature's health is set to 0, its role to 3 and its `+0x6C` to 5000. When the lookup fails, this step does nothing.
- `0x135DEE` (world update): role and `+0x64` are copied from a byte and a 32-bit value of a record (offsets `+0x28` and `+0x2C` of that record), so roles not named above, including 6, can come from that record.
- `0x13B718` (world update): a creature created during an action receives role 3 when the acting creature's role is 0, otherwise role 1.

Other writers of `+0x64` traced: `0x025A23` (the value `0x2C`) and `0x135DF4` (copied from the same record as above).

Toggle word `+0x124`: no instruction in either image modifies `+0x124` in place with the mask `0x20`. The shared bit-setting helper (client `0x02F160`, server `0x005570`) ORs or clears a given mask in the 16-bit word at its object's `+0x114`, and is called with the creature's `+0x10` sub-object. None of its call sites in either image passes the mask `0x20`. Writes that build the word in a register and then store it were not exhaustively traced. The word is written as a whole by several copy routines (for example server `0x0070EF`, `0x01B9AA`, `0x01D85A`, `0x01E05B`, `0x02307A`) and cleared bitwise elsewhere. The producer of bit 5 is therefore not established. The client's player-side routine `0x0A6B50` tests the same bit, as an alternative to role 1, in two of its creature loops. That suggests a combat-side marker but does not prove one.

Reconstruction observations (leads only, not confirmed natively): the reconstruction calls `+0x60` the hostility type, with values 0 player, 1 enemy, 3 friendly non-player character, 4 unnamed, 5 pet and 6 attackable object; it calls `+0x64` the race and describes the 32-value list as peaceful animal races; it calls `+0x7E` the appearance flag word with bits 9, 11 and 12 marking tinted, summoned and mission-level creatures; and it calls `+0x124` a 16-bit toggle word whose bit 5 overrides combat sides. The traced producers above are consistent with role 5 marking a creature linked to a creator key and with summoned creatures inheriting an opposing role, but the game meanings remain unresolved.

### 2.6 A distinct sibling routine

`0x196CA0` / `0x0CFCC0` (read in the client image only; the server routine was located by its predicate calls) reads the same fields and calls the same predicate, but it is a different relation (it has no same-object rule, returns 0 rather than 1 for role 6, and returns 0 when the first creature has role 5 and the second role 0). It is not called by the strike loop and is not specified here.

### 2.7 Callers of the hostility test

Confirmed by the byte scan. In every caller the source is the first argument.

| Client | Server | Generic role |
| --- | --- | --- |
| `0x20F192` | `0x134F52` | World update, target loop before the action dispatch that uses the struck set and the horizontal reach test (role not traced further). |
| `0x21440A` | `0x13A1CA` | Melee strike loop (striker, target); the site specified in section 4. |
| `0x214A3B` | `0x13A7FB` | Area-heal check after a target in the melee strike loop (striker, target); proceeds only when the result is 0. |
| `0x21658E` | `0x13C34E` | Beam case target test (striker, target). |
| `0x219F7D` | `0x13FD3D` | World update, nearby-hostile detection for a list of melee actions while their time is below 200; sets a flag when a hostile living creature is within a size-based radius. |
| `0x22162C` | `0x1473EC` | World update, projectile/object collision loop over creatures (owning creature, target). The test is skipped, and the target remains eligible, when the owner lookup gives none, or when the object's 32-bit `+0x60` is 3 and its byte `+0x64` is 2. |
| `0x2225AA` | `0x14836A` | World update, a second object collision loop over creatures (owning creature, target). When the owner is null the test, and that branch's health check, are skipped. |
| `0x098C6A` | none | Client only: local-player target choice (routine `0x088EE0`); the local player is the source and each candidate the other. |

Client total 8, server total 7. The seven world-update pairs differ by exactly `0xDA240`. The byte scan found no server call corresponding to the client-only site.

## 3. The geometric test (`0x19EE90` / `0x0D4D80`)

This is a line-of-sight test between two fixed-point points, limited by a range. Fully audited apart from its nested ray-march (3.4).

### 3.1 Interface

- The world in the counter register (passed on to the ray-march).
- Four stack arguments, removed by the routine (16 bytes): (1) a pointer to the start point (three signed 64-bit coordinates); (2) a pointer to the end point (same layout); (3) a byte flag, passed on to the ray-march; (4) a binary32 range.
- The result is in the low byte: 1 passes, 0 fails. On the early-return paths only the low byte is defined.

At the melee strike site (server `0x13A1AA`, client `0x2143EA`): the world is the world update's own object (its counter-register argument); the start is a point buffer prepared earlier by the strike case (its contents are given by the held strike contract and were not re-audited here); the end is the target's position; the flag is 1; and the range is 200 (`0x43480000`).

### 3.2 Computation, in order

1. **Difference.** For each axis, the start coordinate is subtracted from the end coordinate as a 64-bit two's-complement subtraction (wrapping).
2. **Conversion.** Each 64-bit difference is loaded exactly into the x87 unit and stored as binary32 (one rounding, to nearest), then multiplied in binary32 by 2^−16 (`0x37800000`). Call the results dx, dy, dz.
3. **Squared length.** q = (dx × dx + dy × dy) + dz × dz, every operation in binary32. The listing adds the y square to the x square; the binary32 sum is the same either way.
4. **Near.** When 0.0001 (binary32 `0x38D1B717`) is greater than q (ordered), the result is 1 and nothing else is done.
5. **Far.** Otherwise the range is squared in binary32; when q is greater than that square (ordered), the result is 0.
6. **Length.** Otherwise q is widened to binary64, its square root is taken by the precise double-precision square-root import (`0x28D9C8` / `0x14A9CE`), and the root is rounded to binary32: call it L.
7. **Direction.** u = (dx ÷ L, dy ÷ L, dz ÷ L), each a binary32 division.
8. **Ray-march.** The ray-march (`0x1A35D0` / `0x0D6730`) is called on the world with: the start pointer, a pointer to u, the maximum distance L, the byte 0, and the byte flag of argument 3. It returns a binary32 distance d (on the x87 stack, stored as binary32).
9. **Result.** The result is 1 when d is greater than or equal to L − 0.01 (the subtraction in binary32; 0.01 is `0x3C23D70A`), ordered; otherwise 0, including when d is NaN.

Because the inputs are integers, q is never NaN or infinite. With the range 200, the squared bound is exactly 40000.

### 3.3 Draws and writes

The routine and everything it reaches (the ray-march, its cell helpers, the square-root import, 64-bit division and the stack-guard helpers) contain no call to `rand`. The only computed calls reached are in the stack-guard failure helpers. The routine writes only its own stack. The ray-march writes only its own stack and, through the column-cell helper (client `0x02F730`; its server counterpart is among the reached helpers), up to two process-wide static fallback cell records used for out-of-range heights; no creature or world state is written.

### 3.4 Ray-march bound (`0x1A35D0` / `0x0D6730`, not fully audited)

Observed behaviour, sufficient to bound the geometric test:

- It walks the world's cells from the cell containing the start point along u, one cell boundary at a time, accumulating the travelled distance from 0.
- Each cell is classified: class 0 when the type held in the low five bits of the cell's fourth byte is 0 or 2, otherwise class 1. Missing zones, coordinates outside the world and heights outside a column use fixed static cells.
- The walk stops at the first cell, including the start cell, whose class differs from the byte argument (0 here, so it stops at the first class-1 cell); after at most 501 cells; or once the travelled distance reaches the maximum. The distance is then lowered to the maximum when it exceeds it.
- When the last byte argument is nonzero (it is 1 at the strike site), a second phase examines the zones between the start and the end and can only lower the distance. Its tests were not audited.
- The returned distance is therefore at most the maximum.

So at the strike site the test passes when the points are within 0.01 world units of each other (q below 0.0001), or when they are at most 200 units apart and the march from the start reaches within 0.01 of the end point without meeting a blocking cell or a second-phase obstruction.

### 3.5 Other callers of the geometric test

Found by the same byte scan, roles not characterised here. Client: `0x02D166`, `0x02D3CE`, `0x0987FE`, `0x099175`, `0x099823`, `0x09A05F`, `0x0A86FA`, `0x0A8C18`, `0x0DB853`, `0x2143EA`, `0x221932`, `0x222872` (12). Server: `0x003586`, `0x0037EE`, `0x01C343`, `0x13A1AA`, `0x1476F2`, `0x148632` (6).

## 4. The melee strike site

At server `0x13A1AA`–`0x13A1D1` (client `0x2143EA`–`0x214411`), after the reach tests:

1. The geometric test is called (3.1). When it fails, the target is skipped.
2. The hostility test is called with the striker first and the target second. When it returns 0, control goes to the area-heal check; otherwise the strike proceeds.

Since the hostility test returns 0 for the striker itself, the striker is never struck by its own loop. When the start point is the striker's own position, the geometric test passes for the striker itself (q = 0). Whether it then reaches the area-heal check depends only on the reach tests of the held strike contract.

## 5. Global state and lookups

The hostility test and its predicate read no global state and perform no lookup, so they cannot fail a lookup. The geometric test reads world cells through the ray-march. A missing zone or an out-of-range coordinate does not fail: it is classified through a fixed static cell. No game mode, player-versus-player setting or local-player id is read by either routine; the client-only caller supplies the local player itself.

## 6. Draws and writes summary

Neither routine, nor anything they reach, makes a random draw. The hostility test writes nothing. The geometric test writes nothing beyond the ray-march's static fallback cells (3.3).

## 7. Client and server

Each item below was established in its own image:

- The hostility test: 63 instructions in each image, identical after masking the predicate's address. No data references.
- The predicate: 72 instructions in each image, identical. No data references.
- The geometric test: 133 instructions in each image, identical after masking. Its three constants have identical bytes: client `0x2FCD94`, `0x345D90`, `0x2FCDA0`; server `0x15869C`, `0x173720`, `0x1586A8`.
- The ray-march: 989 instructions in each image, identical after masking. All 30 data references load identical bytes.
- Each of the seven world-update call sites agrees in a window of 60 instructions on either side, after masking.

No behavioural difference was found. The client has one additional caller (2.7) with no server counterpart.

## 8. Unresolved

- The game meaning of the role values, of the 32-value list at `+0x64`, of bits 9, 11 and 12 of `+0x7E`, and of toggle bit 5 of `+0x124` (reconstruction leads only, 2.5).
- The producer of toggle bit 5, and the full set of producers of the role byte (network and record copies were not traced to their sources).
- The internals of the ray-march's second phase, the exact start-cell rounding for negative coordinates, and the contents of the static fallback cells (3.4).
- The roles of the pre-dispatch loop and the two object collision loops beyond the one-line descriptions in 2.7, and the roles of the geometric test's other callers (3.5).
- The sibling relation at `0x196CA0` / `0x0CFCC0` (2.6) is identified but not specified.
