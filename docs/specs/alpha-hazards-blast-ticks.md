Status: FINAL / STABLE / HELD. Full author readback completed.

# Attack-routine call sites 1, 2 and 5: zone-object hazards, the blast and type-4 buff ticks

This independently authored, code-free contract closes three of the seven world-update call sites of the attack routine end to end: site 1 (the zone-object hazards of types 6, 7 and 8), site 2 (the blast of a dead kind-144 creature) and site 5 (the periodic hits of type-4 buffs). It covers the records and their producers, the loop gates, the geometric tests with their full arithmetic, the damage, vector and flag arguments, and every random draw in order. The held contract `alpha-attack-routine.md` was used only to locate routines and for terms; its descriptions of the attack routine (sections E1–M12) and of the attack-power routine are referenced, not repeated. The Infinite-Cubes reconstruction (`alpha/src/cube/WorldUpdate.cpp`, `WorldGeneration.cpp`, `include/cube/Zone.h`) was used only as a lead for where the loops are. Every fact below was re-established from the original images.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`. Every address is an RVA, written client / server where both are given; where one is given it is the server RVA. Inside the world update routine (client `0x20C510`, server `0x1322D0`) the client lies `0xDA240` above the server. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method and conventions

The routines were read in `decompiled/server/assembly.txt` and `decompiled/client/assembly.txt`. The world update routine has no decompiler output (its decompilation timed out), so only instruction listings were used. Instruction runs that both listings omit (inside the world update: `0x1331DA`–`0x13339E`, `0x13342D`–`0x1335E2`, `0x13630D`–`0x137804`, `0x137F28`–`0x138117`, and the client equivalents) were disassembled from the image bytes where they mattered. Constants were read from image bytes. The draw and reach statements rest on the server call graph `decompiled/server/calls.csv`, followed transitively; library routines were identified from the function list.

Client and server were compared instruction by instruction after replacing absolute addresses; every differing callee, constant and jump target was paired, and each paired constant was read in its own image (section 8).

**Units.** A **raw coordinate** is a signed 64-bit fixed-point value with 65536 units per block. **Presentation units** are a raw value converted to binary32 (round to nearest through the x87 unit) and then multiplied by 2^−16; the conversion routine is `0x02C4A0` / `0x002550` and the scalar form is `0x0120F0` / `0x001420`.

**Draw.** One call to the runtime random function through the import slot `0x2FC468` / `0x1582F4` (as in the held contract). Its value is non-negative, so the signed remainders below are ordinary remainders. A **scaled unit draw with factor k** is the draw converted to binary32, multiplied by k, then divided by 32767.

**Integer division by 200 or 400** is signed 32-bit division truncating toward zero (implemented by a reciprocal multiply with sign correction). Sums feeding these divisions are 32-bit and wrap.

**Raw squared length.** For a raw difference vector, each component is multiplied by itself as a signed 64-bit product, the product is divided by 65536 with truncation toward zero (the runtime's signed 64-bit multiply and divide helpers), and the per-component results are summed in 64-bit wrapping arithmetic. The result is in raw units of squared blocks. The 3-component form is routine `0x03B4A0` / `0x006260` (through `0x03AC20` / `0x0056C0`); the horizontal (x and y only) form is `0x087F80` / `0x01CE90`. The product overflows signed 64 bits when a component difference reaches 2^31.5 raw units (about 46341 blocks); the wrapped value is then used as is.

**Binary32 vector helpers.** The squared length of a binary32 vector is (x·x + y·y) + z·z (`0x024860` / `0x0021B0`). Normalising (`0x0240F0` / `0x001FB0`) recomputes that sum, takes its square root in double precision (the runtime's precise square-root import), rounds it to binary32, forms 1 divided by it, and multiplies x, y and z by that reciprocal. Scaling (`0x012780` / `0x001610`) multiplies each component by one binary32.

## 2. Zone-object records

**Storage.** Each zone holds a vector of 392-byte (`0x188`) zone-object records, begin at zone `+0x0C`, end at zone `+0x10`. The zone's own grid coordinates are two signed 32-bit values at zone `+0x60` and `+0x64`. The 0-based position of a record in this vector is its **object index**.

**Layout** (fields relevant here; established from the initialiser `0x18EAA0` / `0x0C84B0`, the generator writes and the state-packet copy routine):

| Offset | Width | Meaning (role as observed) | Initial value |
| --- | --- | --- | --- |
| `+0x00` | 32 | object type | not written by the initialiser |
| `+0x08` | 3×64 | raw position | not written |
| `+0x20` | 32 | rotation index | not written |
| `+0x24` | 3×f32 | size | not written |
| `+0x30` | 8 | active byte | 1 |
| `+0x34` | 32 signed | transition time | 0 |
| `+0x38` | 32 | unknown | 0 |
| `+0x40` | 64 | occupant creature key | 0 |
| `+0x48` | — | embedded sub-object built by `0x006EF0` (not audited) | — |
| `+0x178`, `+0x17C`, `+0x180` | 32 each | unknown | −1 each |
| `+0x184` | 32 | unknown | 0 |

**State packet.** When the world update changes an object's activity it appends a 0x58-byte record to output-aggregate list `+0x30` (append routine `0x0DD700` / `0x0284F0`, built by `0x0DC8B0` / `0x022D70`). It holds the zone's two grid coordinates and the object index as three 32-bit values, then a copy of the object's `+0x00`–`+0x2C`, `+0x30` (byte), `+0x34`, `+0x38`, `+0x40` and `+0x44`. The packet is built after the activity change, so it carries the new state.

**Activity change** (`0x1A58E0` / `0x0D8C90`; arguments: the object, the output aggregate, the new state byte). If the object's `+0x30` already equals the new state, nothing happens. Otherwise:
1. `+0x34` becomes 1000 minus `+0x34` (32-bit), replaced by 0 when that result is negative.
2. `+0x30` becomes the new state.
3. A presentation record is appended to aggregate list `+0x08` at the object's position in presentation units, with T1 = T2 = 1 and an identifier chosen by type: type 5 → 51 for either state; type 6 → 53, only when activating; type 7 → 52, only when activating; any other type, type 8 included → 54 for either state. When type 6 or 7 is deactivated no record is appended.

It makes no draw. Its only other caller (`0x0CF209`, inside the hit routine) acts on type-5 objects only, so for types 6, 7 and 8 the world-update loop of section 3 is the only activity writer found.

**Producers in world generation.** All callers of the object-append routine (`0x1A0E00` / `0x0D6670`) in the server were examined: 84 sites. Each record's type was traced to an immediate write, a register value or a builder routine (`0x0F2CD0`, `0x0F2EE0`, `0x0F3490`, `0x0F3630`, `0x12A830`). Only one produces any of types 6, 7 or 8: a **type-7** producer in the generation routine server `0x100300` / client `0x1CC510`, window server `0x1058CA`–`0x105A05` / client `0x1D1ADA`–`0x1D1C15` (instruction-identical). For each generated cell it runs in this order:
1. The gate: the cell record's first byte is not 4, bit 0 of its second byte is clear, and the generator's stack byte argument at frame `+0x1C` is zero. If that byte is nonzero, no draw is made here.
2. **One draw**: the trap is placed only when the draw modulo 40 is 0.
3. The record is initialised as in the table, then given: position x = (cell x × 10 + 5 + base x) blocks, y = (cell y × 10 + 5 + base y) blocks, z = (cell z × 10 + 1 + base z) blocks, each shifted to raw units; size (10, 10, 10).
4. **One draw**: rotation `+0x20` = draw modulo 4.
5. Type `+0x00` = 7.
6. **One draw**: `+0x34` = draw modulo 4000.
7. The record is appended to the zone's object vector.

The record therefore starts active (`+0x30` = 1) with a transition time from 0 to 3999. No generation producer of type 6 or type 8 was found in the server. In the client, 85 append sites exist; the type-7 producer was matched, but the remaining client sites were only surveyed, not reconciled one by one. The per-axis cell indices and bases are frame locals whose origin was not traced.

## 3. Site 1 (client `0x20DD18` / server `0x133AD8`)

### 3.1 Clock and frame time

The world update's first stack argument is the frame time Δ (signed 32-bit milliseconds). Early in the routine (`0x20C678` / `0x132438`), world `+0x8000BC` (signed 32-bit, zeroed by the world initialiser `0x0C8570`) is increased by Δ. Call the result the **advanced clock** C. Site 1 reads C, so the clock has already been advanced for this frame.

### 3.2 Zone selection

Zone selection runs at `0x20C8E1` / `0x1326A1`–`0x132AC5`. For every creature in the world collection (world `+0x04`) whose role byte `+0x60` is 0, the routine collects the 3 × 3 block of zones around that creature's zone into a set with no duplicates. The zone grid coordinate is each of the creature's x and y position components converted to an integer by `0x02F570` / `0x005640` (not audited) and then divided by 256, truncating toward zero. Zones are looked up by server `0x006290` (not audited). The outer loop then visits each zone of this set once (`0x20CEE2` / `0x132CA2`). The zone visiting order is that of the set container, which was not audited.

### 3.3 The object loop (`0x20D369`–`0x20DD9C` / `0x133129`–`0x133B5C`)

For each object record of the zone, in vector order, with its object index:
1. **Time.** `+0x34` is increased by Δ, for every type and before any gate.
2. **Occupant.** This step applies only when the 64-bit occupant key `+0x40` is nonzero. The occupant is looked up by key (`0x02F000` / `0x005420`).
   - If the lookup fails, or the occupant's signed `+0x12C` is positive, the object is released.
   - For types 6, 7 and 8 (outside the seat-like type range 16–69 handled by a jump table), the object is also released when any of these holds: the squared length of (occupant position minus object position) in presentation units is greater than 16; the binary32 vector at occupant `+0x40` has a squared length greater than 0; or its action byte `+0x68` is neither `0x53` nor `0x54`.
   - **Release.** If world byte `+0xB4` is nonzero, the rest of this object's processing is skipped. Otherwise the occupant key is zeroed and a state packet is appended.
3. **Host gate.** If world byte `+0xB4` is nonzero, the object's processing ends here (`0x20D8EF` / `0x1336AF`). Everything below runs only when it is zero.
4. **Type gate.** Types other than 6, 7 and 8 end here.
5. **Toggle.** This step applies only when the signed `+0x34` is greater than 2000.
   - If `+0x30` is nonzero, the activity change is applied with new state 0, and a state packet is appended.
   - Otherwise the creatures of the world collection are scanned in order. The first creature that is non-null, has role byte `+0x60` equal to 0, and has a raw squared distance from the object (object position minus creature position `+0x10`) less than 900 × 65536 (closer than 30 blocks) causes the activity change with new state 1 and a state packet. The scan then stops.
   - Because `+0x34` exceeds 2000 here, the activity change always sets `+0x34` to 0.
6. **Active gate.** If `+0x30` is now zero, the object ends here.
7. **Firing gate.**
   - Types 6 and 7 fire when the truncating quotient of (C + Δ) by 200 differs from that of C by 200. This means a multiple of 200 lies in the next frame-length interval above the advanced clock, in the truncation-toward-zero sense.
   - Type 8 fires when `+0x34` equals 0.
   - Otherwise the object ends here.
8. **Hazard loop.** If it fires, every entry of the world creature collection is visited in collection order (an ordered tree; its node holds the 64-bit key at `+0x10` and the creature pointer at `+0x18`; traversal by `0x068D70` / `0x007A30`, not audited). For each entry:
   1. A null creature is skipped.
   2. **Range test.** The raw difference object position minus creature position `+0x10` is formed (`0x02C7A0` / `0x002C50`). Its raw squared length (`0x03B4A0` / `0x006260`) must be less than 25 shifted to raw units, 25 × 65536 = 1 638 400. That is, a three-dimensional distance below 5 blocks. The comparison (`0x02C860` / `0x002D10`) is a signed 64-bit strict less-than.
   3. Creatures with role byte `+0x60` equal to 1 are skipped. This test comes after the range test.
   4. **P1** is formed, in this order:
      1. The vector is (creature position minus object position), converted to presentation units.
      2. Its z becomes 0.
      3. If its squared length is greater than 0 (ordered), it is normalised.
      4. It is scaled by 20.
      5. z becomes 5.
      6. For type 8 the whole vector is then scaled by 2, so z is 10.
   5. **Damage D**, in this order:
      1. L is the level at the object's position (section 3.4), with x and y taken from object `+0x08` and `+0x10`.
      2. L minus 1, in 32-bit, is converted to binary32 and multiplied by 0.25.
      3. The power of two of that value is computed (double-precision power import applied to 2 and the value, through `0x011D10` / `0x0055A0`, rounded to binary32).
      4. The result is multiplied by 10.
      5. For type 8 it is then multiplied by 5.
   6. **Call.** The attack routine is called with these arguments:
      - target: the creature;
      - attacker: null;
      - damage D;
      - flag A: 0; flag B: 0; stun weight: 0;
      - argument 7: a pointer to the object position (never read);
      - vector: P1;
      - the output aggregate and the changed-zone collection;
      - mitigation selector: 1 for type 6 and 0 for types 7 and 8, so type 6 uses resistance and the others use armour;
      - kind K: 0; argument 13: 0;
      - blockable flag: 1.

      Its result is discarded.
9. The object index advances and the next record follows.

**What happens to type 8 after it fires.** The firing path writes no field of the object, and no other `+0x34` write was found for these types apart from the per-frame increase and the activity change. Type 8 fires in the frame in which the toggle activates it, because the activation sets `+0x34` to 0 after that frame's increase. On later frames `+0x34` is Δ or more, so it does not fire again unless Δ is zero or negative and keeps `+0x34` at 0. Once `+0x34` exceeds 2000 the object is deactivated, which sets `+0x34` to 0 and `+0x30` to 0. It reactivates, and fires again, only after a further 2000 ms and only when a role-0 creature is within 30 blocks. Types 6 and 7 follow the same 2-second active and inactive cycle and fire on every 200 ms clock boundary while active. A generated type-7 trap starts active with a phase from 0 to 3999 ms.

### 3.4 Level at a position (`0x19A9B0` / `0x0D2340`)

The receiver is the world. The arguments are raw x and raw y (64-bit each). The result is a signed 32-bit level.
1. **Region column a** = x divided by 65536 (64-bit, truncating), taken as its low 32 bits, then divided by 2048 (32-bit, truncating).
2. **Region row b:** y is converted to double precision, multiplied by 2^−11 and truncated toward zero to a signed 64-bit integer (the runtime's conversion helper `0x28D946` / `0x14A946`). That integer is divided by 65536 (truncating), and the low 32 bits are kept.
3. **Region record lookup** (`0x087DA0` / `0x0286F0`):
   - The lookup fails unless 0 ≤ a < 8192 and 0 ≤ b < 8192.
   - The sector is the pointer at world `+0xBC` + 4 × ((a / 8) × 1024 + b / 8) (`0x034A90` / `0x006210`). A null sector fails.
   - The record is the 104-byte (`0x68`) entry at sector `+0x14018` + 104 × ((b mod 8) + 8 × (a mod 8)).
4. The level is 1 when the lookup fails, or when the record's 32-bit kind at `+0x18` is 0.
5. Otherwise the membership measure m (below) is computed. The level is the record's signed `+0x24` when m ≤ 1 (ordered), and 1 when m > 1 or m is NaN.

**Membership measure** (`0x1F8E10` / `0x12C820`; record fields: centre raw x `+0x00`, centre raw y `+0x08`, radius binary32 `+0x10`, kind `+0x18`):
- If 0.001 is greater than the radius (ordered), m = 0.
- **Kinds 11, 12 and 14.**
  1. dx = x minus the centre x, and dy = y minus the centre y, in 64-bit; each is converted to presentation units.
  2. m = (dy·dy + dx·dx) divided by (radius · radius).
- **All other kinds: warped distance.** The warp factor f is 100 for kind 13 and 256 otherwise.
  1. X′ and Y′ are x and y converted to double, multiplied by −0.0025, and truncated to signed 64-bit integers.
  2. **First noise value N1.** It is computed by the noise routine (`0x0C0EF0` / `0x0D5D30`, not audited) from two doubles:
     - first, (`0x80AD580000` − X′) × 2^−16;
     - second, (`0x1617D0000` − Y′) × 2^−16.

     The 64-bit differences are converted to double through the x87 unit.
  3. N1 is rounded to binary32. u = (the truncation to signed 64 bits of (N1 × f) × 65536, computed in binary32, minus the centre x, plus x), converted to presentation units.
  4. **Second noise value N2.** It uses the same routine with:
     - first, (`0xD5F0000` − X′) × 2^−16;
     - second, (`0x700000` − Y′) × 2^−16.
  5. v = (the truncation of (N2 × f) × 65536, formed in the same way, minus the centre y, plus y), converted to presentation units.
  6. m = (v·v + u·u) divided by (radius · radius).

The level routine and the membership measure make no draws.

### 3.5 Draws at site 1

The site itself makes no draw. Each attack call takes the attackerless path. If the creature's health is greater than zero (or NaN), that path makes **one discarded draw** (E2). Flag A is zero, so M9 makes no draw, and M10 does not apply. The hit routine's draws follow inside the call, as in the earlier contracts. A creature whose health is ≤ 0 returns at E1 with no draw. The loop draws nothing between creatures.

## 4. Site 2 (client `0x20F45D` / server `0x13521D`)

### 4.1 The blast creature and upstream gates

The blast lies in the world update's dead-creature loop (`0x20EB63`–`0x20F66F` / `0x134923`–`0x13542F`).
- **Host gate.** When world byte `+0xB4` is nonzero, control branches at `0x20DDEB` / `0x133BAB` to a different path, and this loop is skipped entirely.
- **Loop.** The loop visits every entry of the world creature collection in collection order.

An entry B is a **blast creature** when all of these hold, tested in this order:
1. B is non-null.
2. Zero is greater than or equal to B's health `+0x16C` (ordered; NaN fails).
3. B's role byte `+0x60` is nonzero. Role 0 skips the whole dead handling.
4. B's signed `+0x6C` equals 0.
5. B's kind `+0x64` equals 144.

When steps 4 or 5 fail, only the common dead handling runs. That handling adds Δ to `+0x6C` (`0x135278`). For role 5 it then runs other logic. For any other role it queues B for removal once `+0x6C` reaches 1000. After a blast this same handling adds Δ to `+0x6C`, so the blast is not repeated on later frames unless Δ is zero or negative.

**How `+0x6C` becomes 0.** The hit routine's fatal branch sets the target's health to 0 and its `+0x6C` to 0 (`0x19618C` / `0x0CF1AC`, instruction-identical). Separately, the per-creature update reduces a kind-144 creature's health by Δ × 0.025 (binary32) each frame while world byte `+0xB4` is zero, replacing a negative result by 0 (`0x211B7B` / `0x13793B`). That decrement does not write `+0x6C`. The same per-creature update adds Δ to `+0x6C` at `0x13E301`, but skips that addition when role byte `+0x60` is 6. Whether a kind-144 creature that dies from this decrement still has `+0x6C` equal to 0 depends on its role and its earlier `+0x6C`. That was not traced.

### 4.2 Work before the victims (one optional draw)

1. **Smoke effect.** A smoke effect record is appended to aggregate list `+0x10`. It holds B's raw position; the vector (0, 0, 20); a colour whose first three components are 1, 0.2 and 0.5 (the fourth was not established); 0.25; the integer 20; the integer 1; and 3 from its initialiser. No draw.
2. **Block removal.** Every block offset (i, j, k) with each of i, j, k from −8 to 8 (k innermost) is visited. The offset is added to B's position, and the offset's squared length in presentation units must be less than 64. Destructible blocks at those positions are removed, each with its own effect and block-change records. No draw.
3. **Removal sound.** If at least one block was removed, a presentation record is appended to aggregate list `+0x08` at B's position. It has identifier 1, T2 = 1 and T1 = the scaled unit draw with factor 0.4, plus 0.5. That is **one draw**. The light and plant updates that follow make no draws.

### 4.3 Victim gates (`0x20F127`–`0x20F49E` / `0x134EE7`–`0x13525E`)

Every entry V of the world creature collection is visited in collection order. B's raw position is copied once, before the loop. V qualifies when all of these hold, tested in this order:
1. V is non-null and V is not B.
2. The hostility test with B first and V second passes (`0x1988A0` / `0x0D18C0`). The test:
   - fails when the two are the same creature;
   - passes when either role byte `+0x60` is 6;
   - passes when exactly one of the two has role 1;
   - passes when both have role 1 and their group values differ;
   - otherwise passes when either has bit `0x20` of `+0x124` set, and fails if not.

   The **group value** (`0x044680` / `0x00F710`) is false when any of bits `0x1A00` of the 16-bit `+0x7E` is set. Otherwise it is true exactly when the kind `+0x64` is one of 19, 20, 22, 23, 25, 26, 27, 30, 31, 32, 33, 34, 35, 53, 55, 56, 57, 58, 67, 68, 74, 87, 89, 92, 93, 98, 100, 106, 107, 145, 146 or 147.
3. V's 64-bit key `+0x08` differs from B's 64-bit `+0x11C0`.
4. V's health is greater than zero. NaN passes.
5. V's signed `+0x128` equals 0.
6. V's key is not in B's key set at `+0x11AC` (lookup `0x077D90` / `0x130600`).
7. **Horizontal reach.**
   1. The reach is V's size x `+0x80` × 0.7 + 8.
   2. Let R2 be the reach multiplied by itself, then multiplied by 65536, all in binary32. R2 is truncated toward zero to a signed 64-bit integer by the runtime conversion helper.
   3. The test (`0x1A7B50` / `0x0DADE0`) passes when the raw horizontal squared length of (B position minus V position), x and y only, is less than that integer. The comparison is signed 64-bit and strict.

   In effect, the horizontal distance must be below the reach in blocks. The helper's result for NaN or out-of-range values was not audited.
8. **Vertical reach.**
   1. H is the absolute value of (B raw z minus V raw z), converted to presentation units. The absolute value is taken through double precision and rounded back.
   2. The test passes when ((V size z `+0x88` × 0.5) + B size z `+0x88`) + 8 is greater than H (ordered).

### 4.4 Arguments and draw (per qualifying victim, in order)

1. **P2** is formed in this order:
   1. The vector is (V position minus B position), converted to presentation units.
   2. z becomes 0.
   3. If its squared length is greater than 0.01 (ordered), it is normalised.
   4. z becomes 0.25.
2. **One draw:** flag A is 1 when the draw modulo 4 is 0, otherwise 0.
3. P2 is scaled by 10, so z is 2.5.
4. **Damage D** is the attack power of B (held contract section 5.6, `0x03DC60` / `0x008F70`) × 25, then × 2 when A is set. The attack power depends on B's multiplier `+0x180`, level, rarity, flags `+0x7E`, striking list and action byte, but not on B's health.
5. V's key is inserted into B's `+0x11AC` set (`0x07F000` / `0x130690`).
6. **Call.** The attack routine is called with these arguments:
   - target: V; attacker: B;
   - damage D; flag A; flag B: 1; stun weight: 0;
   - argument 7: a pointer to B's position (never read);
   - vector: P2;
   - the output aggregate and the changed-zone collection;
   - mitigation selector: 1 (resistance); kind K: 0; argument 13: 0;
   - blockable flag: 1.

   Its result is discarded.

**Draw order for one blast.**
1. The removal-sound draw, if any block was removed.
2. Then, for each qualifying victim in collection order:
   1. the A draw;
   2. the attack routine's own draws for an attacker-present call, as in the held contract: E4 when B's action byte is `0x1C`; one of E5, E6 or E7 when that path applies; M5 makes none because argument 11 is nonzero; M9 when A is set and V's role byte is not 6; M10 when gated;
   3. the hit routine's draws.

No callee of the site makes any other draw.

### 4.5 Lifecycle of the `+0x11AC` set

`+0x11AC` is an ordered set of 64-bit creature keys owned by each creature.
- **Creation and release.** The creature constructor creates it empty (`0x0064BF`–`0x0064E5`), and the destructor releases it (`0x0072D0`).
- **Use at the blast.** At the blast it is read before, and written just before, each victim's attack call. So one victim is hit at most once per blast creature for as long as the set is not cleared, even if the blast condition holds on several frames.
- **Clears.** The only clears found (routine `0x0F8520`) are at `0x212EAE` / `0x138C6E`, `0x2131E7` / `0x138FA7` and `0x218701` / `0x13E4C1`. All three lie in the living part of the per-creature update. That part is bypassed for creatures whose health is ≤ 0 (the gate at `0x211AD1` / `0x137891` branches to the per-creature end, `0x145BE1`). A dead blast creature's set is therefore not cleared by them while it stays dead. Full control-flow reachability of those clears was not proven.
- **Shared use.** The same set is also used by the melee strike (site 3) of a living creature.

## 5. Site 5 (client `0x2183DA` / server `0x13E19A`)

### 5.1 The buff loop

In the per-creature update of a creature P (`0x2120E2`–`0x21843C` with `0x218310`–`0x2184C2` / `0x137EA2`–`0x13E1FC` with `0x13E0D0`–`0x13E282`), the buff list at P `+0x1178` is walked in list order. Each node's entry starts at node `+0x08`: type byte at entry `+0x00`, binary32 value at `+0x04`, signed 32-bit remaining duration at `+0x08`, 64-bit source key at `+0x10`. These are the 24-byte entries of the held contract's M8. For each entry:
1. The duration is decreased by Δ. Call the new value N; the old value is N + Δ.
2. **Expiry.** If N ≤ 0, the entry's position is recorded in a per-creature removal list and nothing else is done for it this frame. In particular a type-4 entry makes no tick in the frame it expires.
3. **Type 1.** If N > 0 and the type is 1, P's signed `+0x12C` is set to 0 when it is positive. No world gate applies.
4. **Type 4.** If N > 0 and the type is 4:
   - The tick is skipped when world byte `+0xB4` is nonzero.
   - Otherwise it ticks when the truncating quotient of (N + Δ) by 400 differs from that of N by 400. The clock operand is the buff's own remaining duration, not the world clock. A tick happens in each frame whose decrease crosses a multiple of 400, at most once per frame however many multiples are crossed.
   - **Tick.** The source is looked up by the key at entry `+0x10` (`0x02F000` / `0x005420`); it may be null. The attack routine is then called with these arguments:
     - target: P; attacker: the source;
     - damage D: the entry's value;
     - flag A: 0; flag B: 0; stun weight: 0;
     - argument 7: a pointer to P `+0x1320` (never read);
     - vector: (0, 0, 0);
     - the output aggregate and the changed-zone collection;
     - mitigation selector: 1 (resistance); kind K: 0; argument 13: 0;
     - blockable flag: 0.

     Its result is discarded.
5. **Other types** do nothing here.

After the walk, every recorded entry is unlinked from P's list and freed, in the order recorded (`0x077C60` / `0x1305B0`), and the list count is decreased.

Next, P's `+0x12C` is decreased by Δ (`0x2184C2` / `0x13E282`). When the result is positive, P's `+0x6C` and action byte `+0x68` are zeroed. So an active type-1 buff leaves `+0x12C` at −Δ for that frame, and that reset does not happen.

### 5.2 Tick count of an M8 buff

An entry created by the held contract's M8 has duration 3000. The truncating quotients by 400 of the values 3000 down to 1 change at 2800, 2400, 2000, 1600, 1200, 800 and 400. The last crossing needs N between 1 and 399. So the buff ticks at most 7 times, fewer when one frame crosses several multiples. It is removed in the frame its duration reaches 0 or below, without a tick in that frame. Each tick deals the stored value (R × 0.1 of the hit that created it) before mitigation.

### 5.3 Draws at site 5

The loop itself makes no draw. Each tick's attack call draws as in the held contract:
- **Null source (attackerless path):** E2 makes one discarded draw when P's health is greater than zero or NaN. M9 and M10 make none.
- **Source present:** E4 draws when the source's action byte is `0x1C`. One draw is made when the E5 miss or E7 absorption path applies. E6 cannot apply, because the blockable flag is 0. M5 and M9 make none. M10 draws when gated.

The hit routine's draws follow inside the call.

## 6. Agreement between client and server

The following are instruction-identical in the two images, apart from absolute addresses, which were all paired (section 8):
- the clock advance;
- the zone selection;
- the zone and object loops, including the occupant path read from the image bytes;
- the host-gated branch around the dead loop;
- the whole dead-creature loop with the blast;
- the buff loop and its type branches;
- the health gate and the kind-144 decrement;
- the three set clears;
- the hit routine's fatal writes of health and `+0x6C`;
- the activity-change routine;
- the level and region routines (`0x19A9B0`/`0x0D2340`, `0x087DA0`/`0x0286F0`, `0x034A90`/`0x006210`) and the membership measure (`0x1F8E10`/`0x12C820`);
- the raw and binary32 vector helpers named above;
- the hostility test and the group value;
- the reach test and the horizontal squared length;
- the vertical helpers (`0x012200`/`0x001490`, `0x0120F0`/`0x001420`, `0x00CBA0`/`0x001CA0`);
- the power wrapper;
- the type-7 producer window.

Every paired constant was read in both images and agrees in value: binary32 0, 0.001, 0.01, 0.025, 0.25, 0.4, 0.5, 0.7, 1, 2, 5, 8, 10, 16, 25, 64, 100, 255, 256, 32767, 65536 and 2^−16; double 2^−16, 2^−11 and −0.0025; and the absolute-value mask. The binary32 constant 20 is an immediate. The integer constants 25, 200, 400, 900, 2000, 1000, 40, 4 and 4000 and the warp offsets are immediates, compared directly. Runtime values that steer these paths, such as world byte `+0xB4` and pointer `+0xB8`, are not established for either program.

## 7. Unresolved

- Game meaning of every field, type, kind, role and identifier. "Trap", "blast", "bomb", "fuse", "occupant" and "region" are descriptive labels only.
- Whether any path other than the object-append routine (zone loading, network receipt on the client) creates zone objects of types 6 or 8. None was found in server generation. The client's append sites were not reconciled one by one.
- The origin of the generator's cell indices, bases and the stack byte at frame `+0x1C`, and the other draws of generation routine `0x100300`.
- The visiting order of the zone set and the creature collection traversal routine (`0x007A30`), the zone lookup (`0x006290`) and the block-to-zone conversion (`0x005640`).
- The noise routine `0x0D5D30` used by the warped membership measure.
- The embedded sub-object at zone-object `+0x48` and the fields `+0x38`, `+0x178`–`+0x184`.
- How a kind-144 creature that dies from the per-frame decrement has its `+0x6C`, and whether such creatures carry role byte 6. The constructor of kind-144 creatures was not located in the original.
- Full control-flow proof that the three `+0x11AC` clears are unreachable for a dead creature. Also unresolved: whether the set is cleared when a creature's health becomes positive again.
- The conversion helper's result for NaN or out-of-range reach values, and the behaviour of the raw squared lengths beyond the 64-bit overflow point.
- The block-removal effect, block-change, light and plant routines of the blast, beyond their making no draws.
- The hit routine's own draws and the consumption of the aggregate lists. These are covered or left open by the earlier contracts.
- Floating-point control state and exceptions.

## 8. Paired addresses (client / server RVA)

Routines:

| Routine | Client / server |
| --- | --- |
| attack | `0x196D30` / `0x0CFD50` |
| hit-routine fatal block | `0x196133` / `0x0CF153` |
| activity change | `0x1A58E0` / `0x0D8C90` |
| object append | `0x1A0E00` / `0x0D6670` |
| object initialiser | `0x18EAA0` / `0x0C84B0` |
| state-packet build / append | `0x0DC8B0` / `0x022D70`; `0x0DD700` / `0x0284F0` |
| creature lookup | `0x02F000` / `0x005420` |
| raw subtraction / addition | `0x02C7A0` / `0x002C50`; `0x02C800` / `0x002CB0` |
| raw squared length | `0x03B4A0` / `0x006260` (dot `0x03AC20` / `0x0056C0`) |
| raw compare against a block count | `0x02C860` / `0x002D10` |
| horizontal squared length | `0x087F80` / `0x01CE90` |
| reach test | `0x1A7B50` / `0x0DADE0` |
| level | `0x19A9B0` / `0x0D2340` |
| region record | `0x087DA0` / `0x0286F0` |
| sector | `0x034A90` / `0x006210` |
| membership | `0x1F8E10` / `0x12C820` |
| noise | `0x0C0EF0` / `0x0D5D30` |
| power wrapper | `0x011D10` / `0x0055A0` |
| presentation conversion | `0x02C4A0` / `0x002550` |
| binary32 squared length | `0x024860` / `0x0021B0` |
| normalise | `0x0240F0` / `0x001FB0` |
| scale | `0x012780` / `0x001610` |
| hostility | `0x1988A0` / `0x0D18C0` |
| group value | `0x044680` / `0x00F710` |
| attack power | `0x03DC60` / `0x008F70` |
| set find / insert | `0x077D90` / `0x130600`; `0x07F000` / `0x130690` |
| list unlink | `0x077C60` / `0x1305B0` |
| random import slot | `0x2FC468` / `0x1582F4` |

Constants:

| Constant | Client / server |
| --- | --- |
| 0.25 | `0x345D98` / `0x1586C8` |
| 10 | `0x345E74` / `0x15873C` |
| 5 | `0x2FCE24` / `0x15872C` |
| 0 | `0x345D84` / `0x158698` |
| 16 | `0x345E78` / `0x158748` |
| 0.01 | `0x2FCDA0` / `0x1586A8` |
| 0.5 | `0x345DA0` / `0x1586D0` |
| 2 | `0x345E10` / `0x158708` |
| 8 | `0x345E70` / `0x158738` |
| 25 | `0x2FCE3C` / `0x158750` |
| 0.7 | `0x2FD570` / `0x1588C0` |
| 0.4 | `0x2FD318` / `0x158810` |
| 64 | `0x2FD330` / `0x15882C` |
| 32767 | `0x2FD338` / `0x158834` |
| 255 | `0x2FCE64` / `0x158778` |
| 0.025 | `0x2FD564` / `0x173C8C` |
| 2^−16 | `0x2FCD94` / `0x15869C` |
| 65536 | `0x2FCE7C` / `0x15878C` |
| 1 | `0x345DC0` / `0x1586DC` |
| 2^−11 (double) | `0x31DB20` / `0x173740` |
| 2^−16 (double) | `0x31DB00` / `0x173730` |
| −0.0025 (double) | `0x31DF90` / `0x173C48` |
| 0.001 | `0x2FD870` / `0x158858` |
| 100 | `0x2FD0B4` / `0x1588E8` |
| 256 | `0x302AB8` / `0x173C14` |
