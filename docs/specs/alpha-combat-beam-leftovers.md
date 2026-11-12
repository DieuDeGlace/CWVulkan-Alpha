Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha ray/beam case: open items left by the aim contract

## Scope and evidence

This contract closes, as far as the original images allow, the points that the held contract `alpha-combat-aim-triplet.md` leaves open for the ray/beam action case (client `0x215C68`, server `0x13BA28`). That contract was read in full and is used only for its terms and to locate routines. Every statement below was checked against the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were verified before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Evidence comes from the original instruction listings (`decompiled/client/assembly.txt`, `decompiled/server/assembly.txt`), the call lists that accompany them, and image bytes (constants, jump tables, import thunks). Sites inside the world creature update exist in the server at the constant offset −`0xDA240`. For every range cited below, the client and server instruction streams were compared after masking absolute addresses, and the referenced constants were compared by value. Helper routines outside the world update have their own server addresses, which are given where they were compared. The isolated Infinite-Cubes reconstruction was consulted once, as a lead for the meaning of `+0x11C0`; this is marked where used.

Terms follow the aim contract: subject S, action `+0x68` (byte), clock `+0x6C` (signed 32-bit), step (signed 32-bit milliseconds of this update), B (`0x03CAA0` with −1), C (`0x047310` with −1), R0 (`0x03D1A0`), aim `+0x160`, position `+0x10/+0x18/+0x20` (signed 64-bit, 65536 units per world unit), fixed conversion, float conversion, gate, first crossing (clock ≤ B < clock + step). These names are added:

- **Mana**: binary32 `+0x170`.
- **Charge**: binary32 `+0x144`. **Stored charge**: binary32 `+0x13B4`.
- **Combo count**: signed 32-bit `+0x70`. **Combo timer**: signed 32-bit `+0x74` (milliseconds).
- **Class pair**: bytes `+0x140` and `+0x141`, written (a, b).
- **Effect list**: the creature's list headed at `+0x1178`; each element begins with a kind byte (element offset `+0x00`).
- **Output**: the second argument of the world update, a record of lists (each list 8 bytes: head and count). Offsets below are within it.
- **Projectile record**: the 112-byte record (constructor `0x059660`) held in world list `+0x14`.
- **rand**: the C runtime `rand` import (client thunk slot `0x2FC468`, server `0x1582F4`; both resolve to `rand` by name). A uniform value u means rand result converted to binary32 and divided by `32767.0` (`0x46FFFE00`).

## 1. Mana at the start of the case

The gate is evaluated first. The mana handling below runs only for subjects that pass the gate. The clamp in 1.1 compares against +0.0 (every path into the dispatch at `0x211E04` provides that zero).

### 1.1 Action 0x68: charge transfer at clock 0 (`0x215C92–0x215CDB`)

When the clock is exactly 0:

1. Mana becomes mana − charge (one binary32 subtraction).
2. If the result is strictly less than 0.0, mana is set to +0.0. NaN is left unchanged.
3. The stored charge `+0x13B4` receives the charge value (the value before step 4).
4. The charge `+0x144` is set to +0.0.

The case then continues to the timing checks in the same update. Action `0x68` never takes the "set to 0 and stop" exit. At any other clock value nothing in this block runs.

### 1.2 Action 0x1C: per-update drain (`0x215D61–0x215D8A`)

Every update in which the case runs for action `0x1C`, mana becomes mana − (s × `0.0002`), where s is the step converted to binary32 (stored once per world update at `0x20C84A`), and `0.0002` is `0x3951B717`. The product is rounded to binary32 and then subtracted. The drain does not depend on the clock window: it also happens before B and after B + C.

### 1.3 Actions 0x5E and 0x5F: cost at the first crossing (`0x215D8F–0x215E95`)

Only on the first crossing (clock ≤ B and clock + step > B; B queried twice):

1. The cost is the binary32 result of the action-cost query `0x044AE0` (server `0x00FB20`), called with the action and −1. Mana becomes mana − cost.
2. The effect list is searched (`0x043F10`, server `0x00EF90`) for the first element whose kind byte is 9. If one exists:
   1. A 24-byte effect value is built: kind byte 9, the 32-bit words at `+0x04`, `+0x08`, `+0x10` and `+0x14` are 0. Bytes `+0x01..+0x03` and `+0x0C..+0x0F` are not written and keep stale stack contents.
   2. That value replaces the existing kind-9 element (set routine `0x046AF0`). For kinds 1, 2, 3, 6, 7, 9, 10 and 11, this routine overwrites the first element of the same kind. For other kinds, or when none exists, it appends.
   3. A 40-byte notice is appended to output list `+0x58` (`0x0460A0`): the subject's 64-bit id at `+0x00` and again at `+0x08`, then the 24-byte effect value at `+0x10`.

**Action-cost query `0x044AE0`.** Index = action − 3. Indices above `0x5C` (unsigned) return 0.0. Otherwise a byte table at `0x044B90` selects one of five outcomes (target table `0x044B7C`):

- Actions `0x03`, `0x04`: 0.1 (`0x3DCCCCCD`).
- Actions `0x05`, `0x1E`, `0x20`, and every action not listed here: 0.0.
- Actions `0x1F`, `0x21`, `0x25`, `0x2B`, `0x2D`, `0x2E`, `0x5F`: 0.0 if the effect list holds any element of kind 9, otherwise 0.3 (`0x3E99999A`).
- Action `0x22`: (1 − (a value from `0x03ED60` with arguments `0x22` and the second argument) × 0.75) × 0.125. This case does not use it.

So the cost for `0x5E` is 0.0. For `0x5F` it is 0.3, or 0.0 while a kind-9 effect is present, and that effect is then reset as above. The server table is byte-identical (`0x00FBD0`).

### 1.4 The negative-mana exit (`0x215EA6–0x215EC6`)

For actions `0x1C`, `0x5E` and `0x5F`, every update in which the case runs reaches this test after the block above, whether or not a deduction happened. If 0.0 is strictly greater than mana (NaN fails the test), mana is set to +0.0 and the rest of the case is skipped for this update: no timing test, no pulse, no ray, no records, no sounds. Control goes to the common tail at `0x211F41`, which still runs the combo timer (Section 2.3). Otherwise the case continues with the timing checks.

Consequences, stated without interpretation:

- For `0x1C` with mana exhausted, mana is driven to 0 and the update is skipped on every following update, unless something else adds more than s × 0.0002 between updates.
- For `0x5F`, if the first-crossing deduction makes mana negative, the first pulse is lost. Later updates find mana at 0, which is not negative, so the 200 ms repeat pulses (aim contract 3.1) still occur.
- For `0x5E`, the cost is 0.0, so the exit is reached only if mana was already negative.

**Random draws:** none in Section 1.

## 2. Combo count, combo timer and subject query 0x047700

### 2.1 Query 0x047700 (client), 0x012150 (server)

This is a pure binary32 query of the subject. It makes no draws and writes nothing. Client and server agree instruction for instruction, and so do its callees: `0x0478B0` / `0x012300`, `0x044CB0` / `0x00FCF0` (jump table `0x044D2C` / `0x00FD6C`), and `0x0C7C00` / `0x014350`. All referenced constants are equal. The result is formed in this order (every operation rounds to binary32):

1. a = 1.0. If byte `+0x60` is nonzero, a = (byte `+0x1A8` as binary32 × 0.0625) + 0.75.
2. If the class pair is (1, 0): ratio = combo count (as binary32) ÷ N (as binary32). If ratio > 1.0 it is replaced by 1.0. Then a = (ratio × 0.5) + a.
3. If the class pair is (3, 1): the same ratio and clamp, then a = ratio + a.
4. t = E + a, where E is query `0x0478B0` (below).
5. The effect list is searched for an element of kind `0x0C`. If none exists, the result is t. If one exists: n = signed `+0x1158` if nonzero. Otherwise, if byte `+0x60` is 0, n = 0. Otherwise n = (signed `+0x190` divided by 2, truncated toward zero) + (byte `+0x1A8` + 1). Then x = (n as binary32 × 0.1) + 1.0, y = 1.0 ÷ x, and the result is ((1.0 − y) + 1.0) × t.

**N (query `0x044CB0`)** is the truncation toward zero of binary32 `+0x17C` × k:

- If byte `+0xAA8` is 3: subtype byte `+0xAA9` 6 gives k = 60; 8 gives 80; 11, 12, 15, 16 and 17 give 20; every other subtype (7, 9, 10, 13, 14 and anything outside 6..17) gives 50.
- Otherwise: k = 20 if bit `0x10` of byte `+0x7E` is set, else 50.

N is computed again for the unclamped branch. It has no side effects. Behaviour when N is 0 (division by zero) is not supplied.

**E (query `0x0478B0`).**

1. L = signed `+0x190` as binary32. x = ((L − 1.0) × 0.05) + 1.0, y = 1.0 ÷ x, z = (1.0 − y) × 3.0.
2. p = binary32 of the precise binary64 power import (`_libm_sse2_pow_precise`) of (2.0, z widened). q is the same power of (2.0, 0.25 × 0.0) and equals 1.0. r = binary32 of power (2.0, 3.0) and equals 8.0.
3. e = ((p × q) ÷ r) × 0.1.
4. Then, in this order, for each equipment slot whose first byte equals the stated kind, e = (slot contribution) + e: `+0x990` kind 3, `+0xAA8` kind 3, `+0x530` kind 4, `+0x648` kind 6, `+0x760` kind 5, `+0x878` kind 7, `+0x418` kind 8, `+0xBC0` kind 9, `+0xCD8` kind 9.

The slot contribution is item query `0x0C7C00`. It returns 0.0 for item kinds other than 3 to 9. Otherwise it is a deterministic function of the item record's bytes `+0x00`, `+0x01`, `+0x0C`, `+0x0D`, its 32-bit `+0x04` (taken modulo 21) and its signed 16-bit `+0x10`, using only power imports. Its full formula is not closed here (Section 10).

### 2.2 The clock rescale after an empty ray loop

In the empty-loop branch (`0x216ADC–0x216B1B`), q₁ = query with the current combo count. Then the combo count is set to 0, and q₂ = the same query. The clock becomes the truncation toward zero (32-bit; an out-of-range value gives `0x80000000`) of (q₁ ÷ q₂) × (clock as binary32). The quotient is rounded first, then the product.

The combo count enters the query only for class pairs (1, 0) and (3, 1). For every other subject, q₁ = q₂ and the ratio is exactly 1.0. The clock is still passed through binary32, which changes it only when its magnitude exceeds 2²⁴. The same rescale sequence appears at every other combo reset listed in 2.3, except the plain increments.

### 2.3 What +0x70 and +0x74 hold, and every write in the world update

The combo count is a signed 32-bit count of successful attack events. The combo timer is a signed 32-bit number of milliseconds since the last increment. Every increment below is followed by a call to `0x0453B0`, which only walks the creature's tree at `+0x139C` and changes nothing.

Writes in the world creature update (client RVA; server −`0xDA240`; all ranges agree):

| Site | Field | When |
|---|---|---|
| `0x211F56` | timer += step | Common case tail `0x211F41`, every update. On the client only for the local player; on the server for every creature. |
| `0x211F75` | count = 0, with rescale | Same tail, when the timer is strictly greater than 4000. Afterwards the tree at `+0x139C` is walked, and for each entry whose key passes `0x287D10` its value is set from `0x03E6A0` (key, −1). The timer itself is not reset here. |
| `0x2134B6` | count = 0, with rescale | Strike case `0x212357`: byte `+0x13B8` is 0, clock + step ≥ B + C, clock < R0, and `+0x128` = 0. |
| `0x2148A2`, `0x2148B2` | count += 1; timer = 0 | Strike case, when the case's local hit flag is set (its setters were not audited here). |
| `0x2169C2`, `0x2169CA` | count += 1; timer = 0 | Ray case, after the loop, if any creature was admitted (aim contract 3.1). |
| `0x216AE9` | count = 0, with rescale | Ray case, empty-loop branch (2.2). |
| `0x220D1F` | owner count = 0, with rescale | Projectile update: a record of type other than 2 and 3 expires (age > 5000) and its owner exists. |
| `0x22230A` | owner count = 0, with rescale | Projectile update: a non-type-3 record touched terrain in this sub-step, consumed no creature, and its owner exists. |
| `0x22235D`, `0x222367` | owner count += 1; owner timer = 0 | Projectile update: a type-2 record had an admitted hit, and its owner exists. |
| `0x222B24`, `0x222B2E` | owner count += 1; owner timer = 0 | Projectile update: an impact had an admitted hit, the record is not type 3, and its owner exists. |

No other instruction in the world update writes `+0x70` or `+0x74` as 32-bit creature fields. The byte-width stores to `+0x70` at `0x21FFCA`, `0x220164`, `0x2202C0` and `0x220381` have a different width and base and were not examined further. **Random draws:** none in these writes.

## 3. Field +0x11C0 (signed 64-bit): ridden-creature id

**Meaning (from its uses).** In the world creature update (`0x21E9B5–0x21EC03`), the id at `+0x11C0` is looked up in the world creature map (`0x02F000`).

- If no creature has that id, nothing happens. The field is **not** cleared.
- If the creature exists and its `+0x16C` is not strictly greater than 0.0, the field is set to 0. A NaN value is treated as alive.
- If it exists and is alive, and the subject's byte `+0x60` is not 5, the subject is attached to it:
  - Position = its position + fixed conversion of (0, 0, 1.5 − its binary32 `+0x1180`).
  - Velocity `+0x34` is set to zero.
  - The triplet at `+0x28` = its triplet + (20.0, 0, 0).
  - The movement triplet `+0x40` is set to zero.
  - Binary32 `+0x1188` = its `+0x1188` × 0.5.
  - `+0x118C` is copied from it.

Other readers:

- The ray exclusion (`0x21626E`).
- The strike and projectile scans (`0x20F1B1`, `0x221642`, `0x2225C0`), each excluding the creature whose id equals the attacker's `+0x11C0`.
- The client controller (`0x0A6E4E`, which finds the ridden creature for control).
- The client pose routine (`0x013463`, `0x013701`).

The isolated reconstruction names the field "pet id while riding". That is a lead only; it agrees with the attachment behaviour above.

**Producers.** Only zeroing stores were found in either image:

- The creature constructor: client `0x03B690` at `0x03B7AA`; server `0x006400` at `0x00651A`.
- The creature reset routine: client `0x046330` at `0x046373`; server `0x0110D0` at `0x011113`.
- The dead-mount clear in the world update (above; client store `0x21E9D9`, server `0x144799`).

No instruction in either image stores a nonzero value to `+0x11C0` or `+0x11C4` by displacement. The only address-taking uses (`0x0A6E5A`, `0x21E9B5`) are a lookup and the clear. The entity network codec does not reference this offset. The reconstruction lead likewise shows only the constructor and reset. A producer, if any, would have to be a whole-record copy or a pointer write not found by displacement search. This is unresolved (Section 10). **Random draws:** none.

## 4. Effect record emitted after an empty ray loop

### 4.1 Emission (`0x2169D9–0x216AD5`; server agrees)

After the sampling loop, set `+0x11AC` is tested for emptiness (`0x077230`). The set was cleared before the loop, so when L ≤ 0 (no pass) it is empty too. If it is empty and the action is not `0x68`, one 72-byte record is built (constructor `0x065ED0` presets `+0x3C` = 0 and `+0x40` = 3.0) and filled:

| Offset | Width | Value |
|---|---|---|
| `+0x00` | 3 × signed 64-bit | S position (`+0x10`, not the sight point) + fixed conversion of (unconditionally normalized aim × L). The product is per component in binary32, with L as in the aim contract. |
| `+0x18` | 3 × binary32 | D × −1.0 (`0x345E8C`), per component. D is the conditionally normalized aim copy of aim contract 3.1. |
| `+0x24` | 4 × binary32 | (1.0, 0.2, 0.5, x). x is copied from an uninitialized stack word. Its value is not defined by the code. |
| `+0x34` | binary32 | 0.1 (`0x3DCCCCCD`). |
| `+0x38` | signed 32-bit | 5. |
| `+0x3C` | signed 32-bit | 1. |
| `+0x40` | binary32 | 3.0 (constructor default). |
| `+0x44` | 4 bytes | Not written (alignment). |

The record is appended to **output list `+0x10`** (`0x086470`). In the same branch the combo reset and clock rescale of 2.2 follow.

**Random draws in the world update:** none.

### 4.2 Consumers

**Client, local presentation** (controller update `0x088EE0`, at `0x0952A5–0x0955AB`). The controller calls the world update once or more per frame (in steps of 20 ms, then the remainder; `0x08CF2E`, `0x08CF49`), all into one output object, and then reads output list `+0x10` in list order. For each record, it repeats `+0x38` times (none if `+0x38` ≤ 0). Each repetition builds one particle and appends it to the controller's particle list at `+0x800740` (`0x0869D0`):

- **Kind:** record `+0x3C`.
- **Position:** record `+0x00`.
- **Velocity:** record `+0x18` + (r₃, r₂, r₁) × (record `+0x40` × 2.0). Each rᵢ = u − 0.5, and the draws are made in the order r₁, r₂, r₃, so the first draw becomes the third component.
- **Second triplet:** set to zero.
- **Third triplet:** (r₆, r₅, r₄) × 100.0, from three further draws in the same pattern.
- **Colour:** the four values at record `+0x24`. If the kind is 1, the colour is replaced by (1.0, u, 0.0, 1.0), using one further draw. This replaces the undefined fourth value for the beam's record.
- **Size:** record `+0x34`.

So each beam effect record costs 5 × 7 = 35 `rand` draws on the client, made after the world update, in the controller.

**Network.** On the server, after its world update (`0x149C50`, call at `0x14A0AE`), the distributor `0x0272D0` copies each output `+0x10` record into a peer's pending list (peer `+0x28`). It does this for every peer whose own creature (found by the peer's id in the world map) satisfies (y² + x²) + z² < 40000.0. Here each component is the 64-bit difference (record position − creature position) converted to binary32 with one rounding and multiplied by 2⁻¹⁶ (`0x15869C` = `0x37800000`; `0x158924` = 40000.0). That radius is 200 world units, with a strict comparison. Output list `+0x08` (sound records) uses the same filter. Lists `+0x00`, `+0x18`, `+0x20`, `+0x28`, `+0x30`, `+0x38`, `+0x40`, `+0x48`, `+0x50` and `+0x58` are passed to per-list copy routines without this inline test (filtering inside those routines was not examined).

The client image contains the same distributor at `0x0DC8E0`, called from `0x09C26D` with the controller's output when the controller's `+0x8006C8` object exists. Its role on the client (for example, local hosting) is not resolved.

A matching reader exists in the client: `0x058860` reads a 32-bit count and then that many raw 72-byte images into such a list. The record therefore travels as its raw bytes, including the undefined colour word. How the receiving client turns received records into particles, and the path that calls `0x058860`, were not traced (Section 10).

## 5. The water cloud (projectile record type 3)

### 5.1 Record contents when created by the ray case

The 112-byte projectile record (constructor `0x059660`) starts with these defaults:

- `+0x00..+0x0F` all bits set; `+0x10` = 0.
- `+0x30`, `+0x34`, `+0x38` = 0.
- `+0x4C` = 0.5, `+0x50` = 1.0, `+0x54` = 1.0, `+0x58` = 0.
- Byte `+0x5C` = 0, `+0x60` = 0, byte `+0x64` = 0, `+0x68` = 0, `+0x6C` = 0.

The ray case then writes:

| Offset | Value |
|---|---|
| `+0x00` | Owner: S 64-bit id (`+0x08`). `+0x08..+0x0F` keep all bits set; `+0x10` keeps 0. Unlike shots, nothing is copied from `+0x1B0`. |
| `+0x18` | Position (3 × signed 64-bit): see 5.2. |
| `+0x3C` | Velocity (3 × binary32): (0, 0, 0). |
| `+0x48` | binary32: (query `0x046E80` of S) × 0.1. |
| `+0x4C` | binary32 radius: 4.0. |
| `+0x50` | binary32: 1.0. |
| `+0x54`, `+0x58` | 0. |
| `+0x5C` | byte 0. |
| `+0x60` | signed 32-bit type: 3. |
| `+0x64` | byte: 2. |
| `+0x68` | signed 32-bit age (ms): 0. |
| `+0x6C` | 0 (default). |

Each record is appended to world list `+0x14` and to output list `+0x20` (`0x0861A0`, two calls).

### 5.2 What reaches it

All paths require class pair (3, 1) on the creature that owns the attack.

1. **Ray case, before the loop** (`0x21603A–0x2161D9`). This is reached in a pulse, after the mana exit and the ray length, for any action of the case (`0x1C`, `0x5E`, `0x5F` and `0x68`), and only on the first crossing (B re-queried). Position = O + fixed conversion of (unconditionally normalized aim × L). O is the sight point, unlike the effect record of Section 4. For `0x68`, L is 10.0. It is created even when L ≤ 0.
2. **Ray case, inside the loop** (`0x2167CD–0x216913`). On the first crossing, after each creature that the admission routine accepts (whatever the hit routine returns), one more record is created with position = the current sample point P. All other fields are as in 5.1.
3. **Impact conversion of water bolts** (`0x222B35–0x222B92`). In shot case `0x217A21`, actions `0x25`–`0x2E` create type-1 records (action `0x6C` creates type 4). When the shooter's class pair is (3, 1), byte `+0x64` is 2; it is 1 for any other second byte with first byte 3, and stays 0 otherwise (`0x21810D–0x218122`). A type-1 record with byte `+0x64` = 2 that has an impact (it consumed a creature or touched terrain in a sub-step) is not removed. Instead it is converted in place:
   - Type becomes 3.
   - Age becomes 3000, or 0 if byte `+0x5C` is nonzero.
   - `+0x48` is multiplied by 0.05.
   - `+0x4C` becomes (`+0x54` × 5.0) + 5.0.
   - `+0x54` becomes 0 and byte `+0x5C` becomes 0.

   Position and velocity are kept.

Because the ray case runs only for gated subjects, on the client clouds of paths 1 and 2 are made only by the local player; on the server, by subjects with byte `+0x60` nonzero (or a subject that is the world's local player).

### 5.3 Lifecycle in the projectile update (`0x220A76` onward, after the creature loop; server agrees)

Each world update, for every record in world list `+0x14`, in list order:

1. **Velocity.** Types other than 1 and 2 (so including 3): velocity z = z − (((step as binary32 × 0.001) × 30.0) × 0.25).
2. **Displacement and age.** The displacement (velocity × (step × 0.001)) is formed. Age += step.
3. **Expiry.** If the type is not 2 and age > 5000, the record is queued for removal. A type-3 record does nothing else on expiry (no owner combo change). It is removed after the loop.
4. **Owner and pass count.** Otherwise the owner is looked up by the id at `+0x00` (none if absent). The pass count n = truncation toward zero of (|displacement| × 2.0 + 1.0). The length is binary32 of the binary64 square root of (x² + y²) + z². If n ≤ 0, the record does nothing more this update. Because a cloud's velocity keeps falling under step 1, n grows with age (for example, from age 0 at a constant 16 ms step, n first becomes 2 after about 4.2 seconds). A converted bolt keeps its flight velocity, so its n can exceed 1 from the start. Each pass repeats steps 5 and 6.
5. **Creature scan.** In each pass, every creature of the world map is visited in map order (Section 6). A creature is skipped if it is null or its `+0x16C` is not strictly greater than 0.0. For a type-3 record with byte `+0x64` = 2, there is **no** owner exclusion, no admission test and no ridden-creature exclusion, so the owner and its allies are candidates. Then:
   - **Box overlap**, all six tests in this order. With h = the candidate's size component × 0.5 and r = record `+0x4C`, both turned into fixed point by × 65536 with truncation:
     - (record + r) ≥ (candidate − h) on x, y and z;
     - then (record − r) < (candidate + h) on x, y and z.
     - The 64-bit comparisons are signed.
   - **Line of sight:** routine `0x19EE90` (world, record position, candidate position, 1, 200.0) must return true.
   - **Dodging candidates:** if the candidate's `+0x128` is nonzero, its id is inserted into the owner's set `+0x11B4` (only if the owner exists), and it is skipped.
   - **Pulse gate:** for type 3, the effect applies only when the signed truncating quotient of (age − step) by 1000 differs from that of age. Since step 3 removed records with age > 5000, a cloud pulses at each 1000 ms boundary up to 4000, and at 5000 only if the age lands exactly on 5000. A converted bolt starting at age 3000 can pulse only at 4000, and at exactly 5000. In a pulse update, every one of the n passes applies the effect again.
   - **Effect** (`0x221B50–0x221EBB`):
     1. crit = false; if the owner exists, crit = crit query `0x0444A0` of the owner (1 draw).
     2. amount = `+0x48`, × 2.0 if crit.
     3. If the owner exists and is the local player, or (server only) is a creature with byte `+0x60` nonzero, the chance query `0x0441A0` (owner, 0.15) is made (1 draw). Its follow-up requires class pair (2, 1) on the owner, so for clouds it never has an effect.
     4. If amount < 0.0, amount = 0.
     5. Because byte `+0x64` is 2: if the owner exists, the hostility test `0x196CA0` (owner, candidate) is false, and the candidate's byte `+0x60` is not 6, a 72-byte hit record (`0x059A30` zeroes bytes `+0x14..+0x46`) is filled:
        - `+0x00` = record `+0x00` (owner id);
        - `+0x08` = candidate id;
        - `+0x10` = −amount, halved when the owner's byte `+0x60` is 1;
        - byte `+0x14` = crit;
        - `+0x20` = candidate position.

        It is appended to output list `+0x00` (`0x086290`). It is then applied through `0x195A60` (world, record, output, …) on the server always, and on the client only when the owner and the candidate are both the local player. A negative amount, that is a heal, is the observed effect. If the owner is absent or the test fails, nothing is applied.
     6. Finally, for every candidate that reached the pulse gate with a pulse, the candidate's signed `+0x134` = max(`+0x134`, record `+0x6C`).
   - A type-3 record is never consumed by a creature.
6. **No movement.** After the scan, a type-3 record goes straight to the next pass. Its position is never advanced and no terrain test is made, so the cloud stays where it was placed or converted.

**Random draws per cloud pulse**, per affected candidate and per pass, in this order:

1. One draw inside `0x0444A0` (if the owner exists).
2. One inside `0x0441A0` (under the owner condition of step 3).
3. Then whatever `0x195A60` draws. That routine reaches `rand` at 93 call sites through its callees; they were not audited.

The candidate visiting order is the map order of Section 6.

## 6. Order of "every other creature in the world map"

The world creature map is the container at world `+0x04`. It is an ordered balanced binary tree with nil-flagged nodes (flag byte at node `+0x0D`). Each node holds a signed 64-bit key (the creature id) at node `+0x10` and the creature pointer at node `+0x18`.

- **Ordering.** The lookup `0x077D90` (server `0x130600`, identical) orders keys by the high 32 bits compared signed, then the low 32 bits compared unsigned. That is ascending signed 64-bit order.
- **Iteration.** Iteration starts at the leftmost node (the head's first link) and advances with the in-order successor (`0x1C3EA0` via `0x068D70`; inlined on the server at `0x007A50`). So the sampling loop visits creatures in **ascending signed 64-bit id**, on both client and server. The world creature loop, the strike scans and the projectile scans use the same order.
- **Hit sets.** The sets `+0x11AC` and `+0x11B4` are the same tree type keyed by signed 64-bit id, searched with the same routine.

Which creatures each side holds in the map (local versus networked) is outside this contract. Whether a hit routine can insert into or remove from the map while the loop iterates was not audited (Section 10).

## 7. The "driven" flag and reachability of the facing and drawn-aim blocks

### 7.1 Setters (complete list; client `0x21A269–0x21A746`, server agrees)

The flag is a per-update local byte. It is cleared at `0x21A269` for every creature that reaches the movement section, which is every creature reaching the facing block (7.2). It is set by exactly these sites, all between the clear and its readers (`0x21F2F1` yaw-from-drawn-aim, `0x21F543` yaw-from-velocity):

| Site | Actions | Condition |
|---|---|---|
| `0x21A381` | `0x44`, `0x45`, `0x5D` | clock > B and clock < B + q, where q = (3 × C) signed-divided by 4, truncated toward zero. |
| `0x21A463` | `0x4D` | clock > B and clock < B + C |
| `0x21A54B` | `0x4E` | clock > B and clock < B + C |
| `0x21A633` | `0x4C` | clock > B and clock < B + C |
| `0x21A65D` | `0x47` | clock < R0 |
| `0x21A690` | `0x48` | clock < R0 (also turns yaw) |
| `0x21A6E7` | `0x44`, `0x45`, `0x5D` | clock > B and clock < R0. The flag is set if this holds, and left as is otherwise. |
| `0x21A746` | `0x39`, `0x4A` | clock < R0 and clock ≥ B |

These sites also set movement vectors and speed values, which are outside this contract. Nothing in the action dispatch writes the flag, and no other instruction in the world update stores to it. Other uses of the same stack slot occur only before the clear or in the projectile update.

### 7.2 Which creatures reach the facing and drawn-aim blocks

The world creature loop skips the whole per-creature body (behaviour, path, action dispatch, movement, facing and drawn aim) in two cases: a null map entry (`0x2104E6`), or `+0x16C` not strictly greater than 0.0 after it is clamped to the maximum from `0x044DB0` (`0x211ADB`). NaN counts as greater. Apart from these, the only branch that bypasses the facing block is at `0x21F0FB–0x21F231`. When flags-word bit `0x0001` is set, byte `+0x5C` bit `0x04` is set and flags-word bit `0x0010` is clear, yaw and head pitch are set by a different rule from `+0x11A0`, and control jumps to `0x21F62B`, past the facing block.

No branch skips the drawn-aim block (`0x21FD54–0x21FDFC`), and neither block tests world `+0xB4` or `+0xB8`. So on the client, every non-null creature with `+0x16C` > 0 reaches the drawn-aim block. Every such creature not in that bypass state reaches the facing block. This includes networked creatures, not only the local player. The server behaves identically. **Random draws:** none in either block.

## 8. Random draws in the ray case: full order (gated subject, one update)

1. **Mana (Section 1):** none.
2. **Ray length** (`0x1A35D0`): no `rand` is reachable through its callees (one computed call into the runtime only).
3. **Cloud before the loop (5.2 path 1):** none.
4. **For each creature passing both geometric tests, in ascending id order:**
   1. One draw inside crit query `0x0444A0` at `0x2163E2` (chance = 1.0 if the effect list holds a kind-11 element, otherwise query `0x03EA40` + binary32 `+0x1190` × 0.15; true when chance > u).
   2. The direct draw at `0x216460`.
   3. For `0x68`, the direct draw at `0x21652A`.
   4. Admission `0x1988A0`: no draws.
   5. Hit routine `0x196D30`: reaches `rand` at 101 call sites; not audited.
   6. The in-loop cloud: no draws.

   The crit draw was not listed by the aim contract.
5. **After the loop:**
   - The effect record and rescale: no draws.
   - Then exactly one of the following, each one draw for pitch:
     - `0x5E` on the first crossing: a sound record at S position, kind `0x28`, pitch u × 0.1 + 1.0, appended to output `+0x08`. The case ends.
     - `0x68` on the first crossing: kind 5 if any creature was admitted, else `0x10`; pitch u × 0.25 + 1.0. The case ends.
     - Otherwise (not `0x5E`): when the signed truncating quotients by 200 of clock + step and of clock differ, kind `0x25` if any admitted, else `0x24`; pitch u × 0.25 + 1.0.
     - `0x5E` outside its first crossing makes no draw here.
6. **Client presentation of the effect record:** 35 draws in the controller after the world update (4.2).

## 9. Client/server agreement

- Every world-update range cited here (`0x215C68–0x216C80`, `0x211F2F–0x212048`, `0x21A1E4–0x21A790`, `0x21E95C–0x21EC08`, `0x213440–0x213500`, `0x214890–0x2148C0`, `0x21F0F1–0x21FE70`, `0x2104C0–0x2104F0`, `0x211A96–0x211AE1`, `0x220A76–0x222B97`) matches the server at −`0xDA240` instruction for instruction after masking absolute addresses. Referenced constants are equal; the only differing slot is the `rand` thunk, and both slots resolve to `rand`.
- These helpers match their server counterparts, including tables:
  - Q `0x047700` / `0x012150`.
  - E `0x0478B0` / `0x012300`.
  - N `0x044CB0` / `0x00FCF0`.
  - Item query `0x0C7C00` / `0x014350`.
  - Cost `0x044AE0` / `0x00FB20`.
  - Kind search `0x043F10` / `0x00EF90`.
  - Crit `0x0444A0` / `0x00F520`.
  - Map lookup `0x077D90` / `0x130600`.
- The output distributors (`0x0DC8E0` / `0x0272D0`) have the same call structure and the same radius constants. Their full bodies were not compared instruction by instruction.
- Client-only: the particle consumer (4.2) and the 72-byte list reader `0x058860` as a receive path.

## 10. Unresolved

- Any producer of a nonzero `+0x11C0` (Section 3), and whether a whole-record copy carries it.
- The internals of item query `0x0C7C00`, crit-base query `0x03EA40`, power query `0x046E80`, and the value from `0x03ED60`.
- Draws and effects inside the hit routines `0x196D30` and `0x195A60`, the hostility test `0x196CA0`, the admission routine `0x1988A0` and the line-of-sight routine `0x19EE90`.
- The client receive path for network effect records, cloud records and hit records; the caller of `0x058860`; and how received effect records are presented, including the undefined fourth colour word.
- The role of the client-side distributor `0x0DC8E0`, and server-side handling when a peer's creature is missing from the map.
- How the kind-9 effect reset of 1.3 later expires or is consumed, and what fields `+0x04`/`+0x08` of an effect element mean.
- The meaning of `0x287D10` and `0x03E6A0` in the combo-timeout branch (the `+0x139C` table).
- Behaviour when N = 0 in query `0x047700`.
- The meaning of fields `+0x128`, `+0x134`, `+0x13B8`, `+0x17C`, `+0x1158`, `+0x1180`, `+0x1188`, `+0x118C`, `+0x11A0` and byte `+0x5C` bit `0x04`, beyond their use here.
- Whether the creature map can change while any of the scans iterate.
- The gameplay meaning of action codes, item kinds and class pairs. Calling the cloud "water" and the heal interpretation follow only from the observed record type, the negative amount and the existing project terminology.
