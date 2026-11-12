Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha creature stat queries left open by the beam contract

## Scope and evidence

This contract closes the creature and item queries that the held contract `alpha-combat-beam-leftovers.md` leaves unresolved: the per-item attack-speed contribution (`0x0C7C00`), the crit-base and crit queries (`0x03EA40`, `0x0444A0`), the power query (`0x046E80`), the action-specific value `0x03ED60`, every query these reach, and the timing queries B (`0x03CAA0`), C (`0x047310`) and R0 (`0x03D1A0`). The beam contract was read in full and is used only to locate routines and for its terms. Every statement below was checked against the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were verified before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Evidence is the original instruction listings (`decompiled/client/assembly.txt`, `decompiled/server/assembly.txt`), the call lists that accompany them, and image bytes (constants, jump and index tables, import slots). Where the listing omits bytes inside a routine (the tail of `0x046E80`), the bytes were read from the image and decoded. For each routine, the client and server instruction streams were compared after masking absolute addresses; constants were compared by value; jump tables were compared by their index bytes and by target offsets relative to the routine start (Section 9). The isolated Infinite-Cubes reconstruction was consulted once, only for item field names; this is marked where used.

### Terms

- **Subject**: the creature the query is asked about. Offsets without another base are creature offsets.
- **Level**: signed 32-bit `+0x190`.
- **Action**: byte `+0x68`.
- **Effect list**: the creature's list whose head pointer is at `+0x1178`. It is a circular doubly linked list with a sentinel. Each node holds the next link at node `+0x00`, and the element begins at node `+0x08` with its kind byte. A search walks from the sentinel's first link, in list order, until it returns to the sentinel.
- **Equipment slots**: item records embedded in the creature at `+0x418`, `+0x530`, `+0x648`, `+0x760`, `+0x878`, `+0x990`, `+0xAA8`, `+0xBC0`, `+0xCD8` (spacing `0x118` bytes).
- **Item record fields** (offsets within one slot): kind byte `+0x00`; subtype byte `+0x01`; unsigned 32-bit `+0x04`; byte `+0x0C`; byte `+0x0D`; signed 16-bit `+0x10`; signed 32-bit `+0x114`. Reconstruction lead only: the isolated reconstruction calls the first six category, id, modifier, rarity, material and level; `+0x114` has no lead name. That naming is not used for any statement below.
- **pow**: the import `_libm_sse2_pow_precise` of MSVCR110 (client slot `0x2FC454`, server slot `0x1582F8`; both resolve to that name). It takes a binary64 base and a binary64 exponent and returns binary64. Every call below uses base 2.0 (binary64 constant). "pow32(x)" means: widen the binary32 value x to binary64, call pow with base 2.0 and exponent x, then round the binary64 result to binary32 (one rounding).
- **rand**: the C runtime `rand` import (client slot `0x2FC468`, server `0x1582F4`).
- Every arithmetic step is a binary32 operation with its own rounding unless stated. Integer-to-binary32 conversions are signed unless stated. Truncation means conversion toward zero to signed 32-bit; an out-of-range or NaN value gives `0x80000000`.

### The level curve

Several queries use the same level curve. Given a binary32 value L:

1. x = ((L − 1.0) × 0.05) + 1.0, with 0.05 = `0x3D4CCCCD`.
2. y = 1.0 ÷ x.
3. z = (1.0 − y) × 3.0.
4. p(L) = pow32(z).

The second factor uses a rarity value R (an integer): w = (R converted to binary32) × 0.25, and q(R) = pow32(w). Where a routine uses R = 0, it does not read a field: it computes 0.25 × 0.0 from two constants and passes that, so q = pow32(0.0) = 1.0.

A third pow call, where present, is pow with exponent 3.0 (binary64 constant), rounded to binary32: 8.0.

Every routine that uses this curve makes its pow calls in the order p, q, then (if present) the 3.0 call. pow calls make no random draws.

## 1. Level helpers 0x045FF0 and 0x045F60

Both take a binary32 level L and a 32-bit integer R (the caller passes a zero-extended byte), and return binary32 through the x87 stack (stored from binary32, so no extra precision).

- **`0x045FF0`** (server `0x010F90`): ((p(L) × q(R)) ÷ e), where e = 8.0 from the third pow call. Three pow calls.
- **`0x045F60`** (server `0x010F00`): p(L) × q(R). Two pow calls; no division.

## 2. Item query 0x0C7C00 (server 0x014350): attack-speed contribution

Input: one item record (an equipment slot). Output: binary32. No draws, no writes.

1. If the kind byte is not one of 3, 4, 5, 6, 7, 8, 9, the result is +0.0 and nothing else is evaluated.
2. **Factor f.** f = 0.2 (`0x3E4CCCCD`) when the kind is 3 and the subtype is one of 5, 6, 7, 8, 10, 11, 15, 16, 17, 18; also f = 0.2 when the kind is 4 (any subtype). Otherwise f = 0.1 (`0x3DCCCCCD`). The subtype is read only for kind 3.
3. **Modifier term m.** r = the unsigned remainder of the 32-bit field `+0x04` divided by 21 (0..20). r is converted exactly to binary32 and m = r ÷ 20.0 (rounded). If byte `+0x0D` equals 12, m = m + 1.0.
4. **Level term.** v = `0x045FF0` with L = signed 16-bit `+0x10` converted to binary32 and R = byte `+0x0C`. That is three pow calls: p(L), q(`+0x0C`), and 8.0.
5. **Result.** c = (v × f) × m, in that order.
6. If 0.001 (`0x3A83126F`) is strictly greater than c, the result is +0.0. Otherwise the result is c. A NaN c is returned as NaN.

Consequences stated without interpretation: c is 0.0 whenever r = 0 and byte `+0x0D` is not 12. The pow calls are made even then (step 4 precedes the product).

Within attack-speed query `0x047700`, the equipment term of E (`0x0478B0`) calls this routine once per slot whose kind byte matches, in the slot order the beam contract gives, adding each result as (contribution + running value).

## 3. Crit queries

### 3.1 Item crit contribution 0x0C6BA0 (server 0x013AC0)

Same structure as Section 2, with these differences:

- Default factor f = 0.05 (`0x3D4CCCCD`). f = 0.1 for kind 3 with a listed subtype (the same list 5, 6, 7, 8, 10, 11, 15, 16, 17, 18) and for kind 4.
- m = 1.0 − (r ÷ 20.0), with r as in Section 2. If byte `+0x0D` equals **11**, m = m + 1.0.
- Level term, product order, and the 0.001 floor are the same as Section 2 (it calls `0x045FF0` with the same arguments).

### 3.2 Crit-base query 0x03EA40 (server 0x009AC0)

Output: binary32. No draws, no writes.

1. b = level curve with L = level (as binary32) and R = 0: p(L), then q = 1.0, then e = 8.0. b = ((p × q) ÷ e) × 0.1.
2. Then, in this order, for each slot whose kind byte equals the stated kind, b = (item crit contribution of that slot, Section 3.1) + b:
   `+0x990` kind 3; `+0xAA8` kind 3; `+0x530` kind 4; `+0x648` kind 6; `+0x760` kind 5; `+0x878` kind 7; `+0x418` kind 8; `+0xBC0` kind 9; `+0xCD8` kind 9.
3. The result is b.

The slot order and kinds are the same as those of the attack-speed equipment term.

### 3.3 Crit query 0x0444A0 (server 0x00F520)

Output: a boolean (0 or 1 in the low byte of the return register). Exactly **one** `rand` draw per call, on every path. No writes.

1. The effect list is searched for the first element whose kind byte is 11 (`0x0B`).
2. If one exists, chance = 1.0, and the crit-base query is **not** evaluated.
3. Otherwise chance = b + (binary32 `+0x1190` × 0.15), where b is the crit-base query (3.2) and 0.15 = `0x3E19999A`. The product is rounded first, then the sum.
4. One `rand` call. u = (rand result converted to binary32) ÷ 32767.0 (`0x46FFFE00`).
5. The result is true when chance is strictly greater than u. A NaN chance gives false.

Consequence: with a kind-11 effect present, the result is false only when rand returns 32767 (u = 1.0 exactly).

## 4. Action-specific value 0x03ED60 (server 0x009DE0)

Inputs: the subject, an action code A (32-bit), and a signed 32-bit second argument k. Output: binary32. No draws, no writes. Both arguments are removed by the routine.

1. **Slot index.** The index query `0x03C9C0` (server `0x007CC0`) maps A to an index:
   - 6 for actions `0x15`, `0x22`, `0x30`, `0x36`, `0x58`;
   - 7 for actions `0x32`, `0x4F`, `0x56`, `0x67`;
   - 8 for actions `0x31`, `0x60`, `0x61`, `0x63`, `0x64`, `0x65`, `0x66`;
   - −1 for every other value (including all values below `0x15` and above `0x67`, compared as unsigned after subtracting `0x15`).
2. If the index is negative, the result is 1.0 and nothing else is read.
3. **Count n.**
   - If k is greater than 0, n = k.
   - If k is negative, n = the signed 32-bit value at `+0x1138` + 4 × index (so `+0x1150`, `+0x1154` or `+0x1158`). If that value is nonzero, it is n.
   - If k is 0, or k is negative and the stored value is 0: if byte `+0x60` is 0, n = 0; otherwise n = (level divided by 2, truncated toward zero) + (byte `+0x1A8` + 1), in 32-bit integer arithmetic.
4. **Result.** t = (n as binary32 × 0.1) + 1.0; s = 1.0 ÷ t; the result is 1.0 − s.

So n = 0 gives 0.0, and the result approaches 1.0 as n grows.

### 4.1 Use in the action-cost query 0x044AE0

For action `0x22`, the cost query (server `0x00FB20`) calls `0x03ED60` with A = `0x22` and with k equal to the cost query's own second argument, unchanged. The index for `0x22` is 6, so with k = −1 (as in the beam case) n comes from `+0x1150`, falling back as in step 3. The cost is then (1.0 − (value × 0.75)) × 0.125, with 0.75 = `0x3F400000` and 0.125 = `0x3E000000`; the product, then the difference, then the second product are each rounded.

## 5. Power query 0x046E80 (server 0x011AD0)

Output: binary32. No draws. It builds and frees a temporary list, and writes nothing else.

1. **Base.** p = p(level as binary32); q = pow32(0.25 × 0.0) = 1.0. s = binary32 `+0x180` × (q × p). There is no third pow call and no division by 8 in this term.
2. **Held-weapon list.** The weapon-list routine `0x03DF60` (server `0x009270`; Section 5.1) builds a list of slot references.
3. **If the list is not empty:** for each listed slot in list order, s = (weapon power of that slot, Section 5.2) + s. Steps 4 and 5 are skipped.
4. **If the list is empty and bit `0x08` of the 16-bit word `+0x7E` is set:** a = pow32 of the level-curve exponent (p of the level, recomputed), then b = pow32(byte `+0x1A8` as binary32 × 0.25). s = ((a × b) × 2.0) + s.
5. **If the list is empty and bit `0x10` of the word `+0x7E` is set:** the same a and b are recomputed (two more pow calls), and s = ((a × b) × 8.0) + s, where s already includes step 4 if it applied.
6. The list nodes are freed and the result is s.

pow call order: p, q; then, only on the empty-list path, two calls for step 4 (if bit `0x08`) and two for step 5 (if bit `0x10`).

In the beam contract's water cloud, the record's `+0x48` is this result × 0.1.

### 5.1 Weapon-list routine 0x03DF60

It returns a list (head and count) of references to slots `+0x990` and `+0xAA8`, appended at the end in the order stated. Rules, in order of precedence:

1. If slot `+0xAA8` has kind 3 and subtype 6 or 7: the list is exactly that slot.
2. Otherwise, if slot `+0x990` has kind 3 and subtype 6 or 7: the list is exactly that slot.
3. Otherwise the action byte selects one of four rules (index = action − 2, compared unsigned; values above `0x5E` use rule D, so actions `0x00`, `0x01` and above `0x60` use rule D). Table at `0x03E130` with targets at `0x03E120`.
   - **Rule A, first slot only:** actions `0x02`, `0x04`, `0x07`, `0x08`, `0x0A`, `0x12`, `0x27`, `0x29`. Slot `+0x990` if its kind is 3.
   - **Rule B, both slots:** actions `0x0B`, `0x0C`, `0x10`, `0x11`, `0x14`, `0x36`, `0x56`, `0x5B`, `0x60`. Slot `+0x990` if its kind is 3, then slot `+0xAA8` if its kind is 3.
   - **Rule C, second slot only:** actions `0x28`, `0x2A`. Slot `+0xAA8` if its kind is 3.
   - **Rule D, all other actions:** slot `+0xAA8` if its kind is 3; then slot `+0x990` if its kind is 3 and its subtype is 12.

The list may be empty. Its order fixes the order of additions in step 3 of Section 5.

### 5.2 Weapon power 0x0C7F60 (server 0x014550)

Input: one item record. Output: binary32. No draws.

1. If the kind byte is not 3, the result is +0.0.
2. L' = (signed 16-bit `+0x10` as binary32) + ((signed 32-bit `+0x114` as binary32) × 0.1). The product is rounded first, then the sum.
3. g = `0x045F60` with L = L' and R = byte `+0x0C` (two pow calls: p(L'), q(R)).
4. The result is g × M, with M chosen by subtype byte `+0x01`:
   - 2.0 for subtypes 3, 4, 13;
   - 4.0 for subtype 5;
   - 8.0 for subtypes 6, 7, 8, 10, 11, 15, 16, 17, 18;
   - 4.0 for every other subtype (0, 1, 2, 9, 12, 14, and 19 and above).

   The subtype dispatch uses an index table at `0x0C811C` (subtypes 3..13) and targets at `0x0C810C`; the remaining subtypes are tested against an explicit list.

## 6. Timing queries B, C and R0

All three are pure 32-bit integer queries of the subject. They make no draws and write nothing. Where a value is "scaled by K", the result is the truncation of K ÷ (Q × `+0x17C`), where Q is the attack-speed query `0x047700` of the subject and `+0x17C` is binary32. The product is rounded first, then the quotient; Q is evaluated once for each such term. All K are binary32 constants with exact integer values. Where a value is "fixed", Q is not evaluated.

So a larger Q, or a larger `+0x17C`, shortens every scaled time. A Q or `+0x17C` of 0 gives an infinite quotient, and the truncation then yields `0x80000000`.

### 6.1 B: wind-up 0x03CAA0 (server 0x007DB0)

Argument: a signed 32-bit action. If it is negative, the action byte is used instead. Values above `0x69` (compared unsigned) take the default. Index table `0x03CEB0`, targets `0x03CE24`.

| B | Actions |
|---|---|
| fixed 0 | `0x00`, `0x08`, `0x0B`, `0x1C`, `0x32`, `0x37`, `0x3C`, `0x3D`, `0x3E`, `0x60`, `0x62`, `0x68` |
| fixed 100 | `0x30`, `0x65` |
| fixed 200 | `0x47`, `0x48` |
| fixed 400 | `0x36` |
| fixed 5000 | `0x69` |
| scaled 50 | `0x06`, `0x07`, `0x17`, `0x18`, `0x19`, `0x1B`, `0x24`, `0x3B`, `0x3F`, `0x40` |
| scaled 100 | `0x02`, `0x03`, `0x04`, `0x05`, `0x0A`, `0x0E`, `0x11`, `0x12`, `0x13`, `0x14`, `0x15` |
| scaled 200 | `0x0C`, `0x0D`, `0x0F`, `0x10`, `0x27`, `0x28`, `0x29`, `0x2A`, `0x43` |
| scaled 300 | `0x01`, `0x09`, `0x1A`, `0x41`, `0x42`, `0x44`, `0x45`, `0x46`, `0x49`, `0x4A`, `0x4B`, `0x4C`, `0x4D`, `0x4E` |
| scaled 400 | `0x16`, `0x1D`, `0x23`, `0x2F`, `0x31`, `0x33`, `0x34`, `0x35`, `0x38`, `0x4F`–`0x56`, `0x58`, `0x5A`, `0x5C`, `0x61`, `0x63`, `0x64`, `0x66`, `0x67`, and every value above `0x69` |
| scaled 500 | `0x26`, `0x2C`, `0x5E` |
| scaled 600 or 1200 | `0x25`, `0x2B`, `0x59`: 600 when byte `+0xAA9` (the subtype of slot `+0xAA8`) is 12, otherwise 1200. The kind byte is not tested. |
| scaled 800 | `0x1E`, `0x20`, `0x39`, `0x3A`, `0x5D` |
| scaled 1000 | `0x5B`, `0x5F` |
| scaled 1200 | `0x2D`, `0x2E` |
| scaled 1600 | `0x1F`, `0x21`, `0x22` |
| scaled 5000 | `0x57` |

### 6.2 C: active window 0x047310 (server 0x011D60)

Argument as for B. Values above `0x68` take the default (scaled 300). Index table `0x047628`, targets `0x0475A8`.

| C | Actions |
|---|---|
| fixed 0 | `0x00`, `0x31` |
| fixed 100 | `0x36` |
| fixed 200 | `0x0A` |
| fixed 500 | `0x32`, `0x4C`, `0x4D`, `0x4E`, `0x60` |
| fixed 600 | `0x1E`, `0x20`, `0x49`, `0x5D` |
| fixed 1000 | `0x44`, `0x45` |
| fixed 1200 | `0x1F`, `0x21` |
| fixed 2000 | `0x5F` |
| fixed 3000 | `0x47` |
| fixed 5000 | `0x48`, `0x56` |
| fixed 6000 | `0x5B` |
| scaled 50 | `0x16`, `0x17` |
| scaled 100 | `0x03`, `0x04`, `0x25`, `0x2B`, `0x3E` |
| scaled 150 | `0x06`, `0x07`, `0x12`, `0x13` |
| scaled 200 | `0x01`, `0x02`, `0x09`, `0x0D`, `0x0E`, `0x0F`, `0x43` |
| scaled 400 | `0x05`, `0x0C`, `0x10`, `0x11`, `0x14`, `0x15`, `0x41`, `0x42` |
| scaled 500 | `0x2D`, `0x2E`, `0x37`, `0x5E` |
| scaled 1200 | `0x1A`, `0x22` |
| scaled 300 | `0x0B`, `0x26`–`0x2A`, `0x2C`, `0x4B`, `0x68`; also `0x08`, `0x18`, `0x19`, `0x1B`, `0x1C`, `0x1D`, `0x23`, `0x24`, `0x2F`, `0x30`, `0x33`, `0x34`, `0x35`, `0x38`, `0x39`, `0x3A`, `0x3B`, `0x3C`, `0x3D`, `0x3F`, `0x40`, `0x46`, `0x4A`, `0x4F`–`0x55`, `0x57`, `0x58`, `0x59`, `0x5A`, `0x5C`, `0x61`–`0x67`, and every value above `0x68` |

### 6.3 R0: total 0x03D1A0 (server 0x0084B0)

R0 takes no argument; it always uses the action byte. It is the 32-bit integer sum (wrapping) of three terms, each evaluated in this order:

1. **The B term**: the same table as 6.1, inlined (index table `0x03DA54`, targets `0x03D9C8`), for the action byte.
2. **The C term**: the same table as 6.2, inlined (index table `0x03DB40`, targets `0x03DAC0`).
3. **A recovery term D** (index table `0x03DBEC`, targets `0x03DBAC`; values above `0x68` take the default, scaled 500):

| D | Actions |
|---|---|
| fixed 0 | `0x30` |
| fixed 100 | `0x00`, `0x32`, `0x60` |
| fixed 200 | `0x47`, `0x48` |
| fixed 400 | `0x36` |
| fixed 600 | `0x0A` |
| scaled 10 | `0x17` |
| scaled 100 | `0x0B`, `0x16`, `0x1A`, `0x1E`–`0x22`, `0x25`–`0x2E`, `0x3C`, `0x3D`, `0x43`, `0x5E`, `0x5F`, `0x68` |
| scaled 200 | `0x07`, `0x0E`, `0x12`, `0x41`, `0x42` |
| scaled 300 | `0x03`, `0x04`, `0x05`, `0x39`, `0x3A`, `0x3E` |
| scaled 400 | `0x0F` |
| scaled 800 | `0x44`, `0x45`, `0x5D` |
| scaled 500 | every other action, including `0x01`, `0x02`, `0x06`, `0x08`, `0x09`, `0x0C`, `0x0D`, `0x10`, `0x11`, `0x13`, `0x14`, `0x15`, `0x18`, `0x19`, `0x1B`–`0x1D`, `0x23`, `0x24`, `0x2F`, `0x31`, `0x33`–`0x35`, `0x37`, `0x38`, `0x3B`, `0x3F`, `0x40`, `0x46`, `0x49`–`0x5C`, `0x61`–`0x67`, and every value above `0x68` |

The result is D + C + B. Up to three Q evaluations are made, one for each scaled term; since Q is pure, they give the same value.

For the beam actions of the held contracts: B is 0 for `0x1C` and `0x68`, scaled 500 for `0x5E` and scaled 1000 for `0x5F`. C is scaled 300 for `0x1C` and `0x68`, scaled 500 for `0x5E` and fixed 2000 for `0x5F`. D is scaled 500 for `0x1C`, and scaled 100 for `0x5E`, `0x5F` and `0x68`.

## 7. Random draws

- The crit query `0x0444A0` makes exactly one `rand` call per call, on every path, after its chance is formed.
- No other routine in this contract calls `rand`, directly or through its callees: `0x03EA40`, `0x0C6BA0`, `0x0C7C00`, `0x045FF0`, `0x045F60`, `0x03ED60`, `0x03C9C0`, `0x044AE0`, `0x046E80`, `0x03DF60`, `0x0C7F60`, `0x03CAA0`, `0x047310`, `0x03D1A0`. Their callees are the pow import, the attack-speed query `0x047700` and its callees, the list primitives, and the allocator. This was confirmed by a transitive walk of the client call list from each routine: the only imports reached are pow, `rand` (from `0x0444A0` only), and the allocator and its failure handlers (from `0x046E80`). None of these routines makes a computed call.

## 8. Precision notes

- Every intermediate is rounded to binary32, except the pow argument (binary32 widened exactly to binary64), the pow result (binary64, then rounded once to binary32), and the unsigned remainder conversion in Sections 2 and 3.1 (exact).
- Results leave these routines through the x87 stack, but always reloaded from a binary32 memory value, so they carry no extended precision.
- Rounding is round-to-nearest-even under the default SSE control state. None of these routines changes the control state.
- Exact parity requires the binary64 pow results of the MSVCR110 precise pow for base 2.0. For integer exponents (0.0 and 3.0) the result is exact. For the non-integer exponent z, the binary32 result depends on the binary64 result being correctly rounded, or at least on its rounding to binary32 being the same; this is not proven here (Section 10).

## 9. Client/server agreement

Each pair below was compared instruction by instruction after masking absolute addresses; there were no differences. The referenced constants are equal by value (only the 4 or 8 bytes each instruction reads were compared), and the jump and index tables are byte-identical with equal target offsets relative to the routine start.

| Routine | Client | Server |
|---|---|---|
| Item attack-speed contribution | `0x0C7C00` | `0x014350` |
| Item crit contribution | `0x0C6BA0` | `0x013AC0` |
| Level helper (with ÷ 8) | `0x045FF0` | `0x010F90` |
| Level helper (no division) | `0x045F60` | `0x010F00` |
| Crit base | `0x03EA40` | `0x009AC0` |
| Crit | `0x0444A0` | `0x00F520` |
| Action-specific value | `0x03ED60` | `0x009DE0` |
| Action → index | `0x03C9C0` (tables `0x03C9F8`, `0x03CA08`) | `0x007CC0` (`0x007CF8`, `0x007D08`) |
| Action cost | `0x044AE0` (`0x044B7C`, `0x044B90`) | `0x00FB20` (`0x00FBBC`, `0x00FBD0`) |
| Power | `0x046E80` | `0x011AD0` |
| Weapon list | `0x03DF60` (`0x03E120`, `0x03E130`) | `0x009270` (`0x009430`, `0x009440`) |
| Weapon power | `0x0C7F60` (`0x0C810C`, `0x0C811C`) | `0x014550` (`0x0146FC`, `0x01470C`) |
| B | `0x03CAA0` (`0x03CE24`, `0x03CEB0`) | `0x007DB0` (`0x008134`, `0x0081C0`) |
| C | `0x047310` (`0x0475A8`, `0x047628`) | `0x011D60` (`0x011FF8`, `0x012078`) |
| R0 | `0x03D1A0` (B: `0x03D9C8`/`0x03DA54`; C: `0x03DAC0`/`0x03DB40`; D: `0x03DBAC`/`0x03DBEC`) | `0x0084B0` (`0x008CD8`/`0x008D64`; `0x008DD0`/`0x008E50`; `0x008EBC`/`0x008EFC`) |

For `0x046E80`, the client listing omits part of the routine's tail (list freeing and return). Those bytes were decoded from the image and match the server's listed tail; the value returned is the running sum s.

The pow and rand imports resolve by name in both images. The server pairs were found by call structure (callers of pow and of the attack-speed query, and callees of already-paired routines) and then confirmed by the comparison above.

## 10. Unresolved

- Whether the MSVCR110 precise pow returns a correctly rounded binary64 for base 2.0 and the non-integer exponents produced by the level curve, and so whether a different correctly rounded pow reproduces every binary32 result. Not tested here.
- The meaning of `+0x180` (power scale), `+0x17C` (speed scale), `+0x1190` (crit bonus), `+0x1A8`, `+0x60`, the word `+0x7E` bits `0x08` and `0x10`, and the array `+0x1138` (indices 6, 7, 8 used here), beyond their use in these formulas.
- The meaning of item fields `+0x04`, `+0x0C`, `+0x0D`, `+0x10`, `+0x114` and the kind and subtype numbers. The names in the Terms section are reconstruction leads only.
- The gameplay meaning of the action codes and of the B, C and D terms. "Wind-up", "active window" and "recovery" are descriptive labels from their use as timing thresholds in the held contracts; they are not proven names.
- Behaviour when Q × `+0x17C` is 0, negative or NaN, beyond the truncation rule stated in Section 6.
- Other callers of `0x045FF0` (for example `0x0C7A90`) and the other level-curve queries that call pow (`0x03CFF0`, `0x03DC60`, `0x03E190`, `0x044DB0`, `0x0467A0`) were not examined.
- What adds or removes kind-11 effect elements (which force the crit chance to 1.0), and the meaning of their other fields.
