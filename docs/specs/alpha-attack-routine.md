Status: FINAL / STABLE / HELD. Full author readback completed.

# Attack routine, its world-update callers and the heal amounts

This independently authored, code-free contract specifies the attack routine that builds hit records and passes them to the hit-application routine. It covers the routine's decision order and every random draw, how the main-hit damage is formed, every field written into the 72-byte hit record, the seven world-update call sites, and the heal amounts sent straight to the hit routine. The contracts `alpha-fatal-disposal-callers.md` and `alpha-fatal-branch-order.md` were used only to locate routines and for terms. The Infinite-Cubes reconstruction (`alpha/src/cube/WorldUpdate.cpp`) was used only to find which world-update region each call site belongs to. Every fact below was re-established from the original images. Where a context label below comes from that lead and was not confirmed, it is marked as such.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`. Every address is an RVA, written client / server where both are given; where only one is given it is the server RVA. The client attack routine lies `0xC6FE0` above the server one, and the client world-update sites lie `0xDA240` above the server ones. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method

The routines were read in `decompiled/server/assembly.txt` and `decompiled/client/assembly.txt`. Server decompiler output was used only as a reading aid for the attack routine; every statement was checked against the instruction listing. Client and server were compared instruction by instruction after replacing absolute addresses (section 9). Instruction runs that the client listing omits were read from the client image bytes. Every differing absolute address (callee, constant, jump table, import slot) was paired and resolved in its own image. Constants and switch tables were read from image bytes. Jump tables were compared as offsets from their routine's start.

**Draw.** One call through the random import slot at `0x2FC468` / `0x1582F4` (`MSVCR110.dll!rand`). A byte scan of the attack routine found exactly eight direct call sites to that slot in each image: client `0x196E20`, `0x196EF9`, `0x1971C0`, `0x197478`, `0x197D1B`, `0x197E3D`, `0x198258`, `0x198545`; server `0x0CFE40`, `0x0CFF19`, `0x0D01E0`, `0x0D0498`, `0x0D0D3B`, `0x0D0E5D`, `0x0D1278`, `0x0D1565`.

**Negative results.** Statements that a callee makes no draw, or does not reach the hit, attack or world update routine, rest on the server call graph `decompiled/server/calls.csv`, followed transitively. Client callees were then shown to be instruction-identical. The searches stopped at the hit routine, whose own draws are specified in the earlier contracts. The only unresolved indirect calls found were inside the runtime's security-cookie failure path.

**Arithmetic.** All arithmetic is binary32 unless a step says otherwise. "Ordered" comparisons treat NaN as failing. Products are written in evaluation order, because that order fixes the rounding. A **unit draw** is a draw converted to binary32 and divided by 32767. A **scaled unit draw with factor k** is the draw converted to binary32, times k, then divided by 32767. **Power of two of x** means the double-precision power import (`0x2FC454` / `0x1582F8`, reached through thunk `0x28D8D4` / `0x14AB14`) applied to base 2 and to x widened to double, with the result rounded to binary32. The **level curve** of a binary32 L is 1 minus 1 divided by ((L minus 1) times the nearest binary32 to 0.05, plus 1). The routine `0x03CA60` / `0x007D60` and several inline copies compute it.

**Presentation record.** A 24-byte record appended to output-aggregate list `+0x08`. It holds three binary32 coordinates, a 32-bit identifier and two binary32 values T1 and T2. Each coordinate is formed from a raw signed 64-bit actor coordinate, rounded to binary32 and multiplied by 2 to the power −16. The conversion routine `0x02C4A0` / `0x002550` does this, and some sites do it inline.

## 2. Interface of the attack routine (`0x196D30` / `0x0CFD50`)

The receiver is the world. The routine takes fourteen 32-bit stack arguments and removes 56 bytes on return. It returns a byte in the low result register: 1 when it reached its end on the main-hit path, or ended because absorption consumed all damage; 0 on every other exit. Arguments, by position:

| # | Meaning (role as observed) | Use |
| --- | --- | --- |
| 1 | target actor | always |
| 2 | attacker actor, or null | null selects the attackerless path |
| 3 | damage D (binary32) | always |
| 4 | flag A (byte) | record `+0x14`; presentation identifiers |
| 5 | flag B (byte) | record `+0x44`; item-extension effects |
| 6 | stun weight W (binary32) | stun chance |
| 7 | pointer | never read |
| 8 | pointer to vector V (three binary32) | records; effect vectors |
| 9 | output aggregate | lists `+0x00`, `+0x08`, `+0x10`, `+0x58` |
| 10 | changed-zone collection | passed to the hit routine |
| 11 | mitigation selector (byte) | resistance when nonzero, armour when zero |
| 12 | kind K (32-bit) | 0x11 and 0x1C are tested |
| 13 | pointer | never read |
| 14 | blockable flag (byte) | block gate; post-hit block bookkeeping |

**Hit list append.** Each hit record goes to the output aggregate's list at `+0x00` (count at `+0x04`, limit `0x3333332`). The append copies offsets `+0x00`–`+0x14`, `+0x18`–`+0x44`, `+0x45` and `+0x46` member by member, so bytes `+0x15`–`+0x17` and `+0x47` are not carried. The server copy routine is `0x022A00`; its client counterpart was not compared separately.

**Hit-record initialiser** (`0x059A30` / `0x022A90`). It zeroes byte `+0x14`, the 32-bit words `+0x18` to `+0x40` and bytes `+0x44`, `+0x45`, `+0x46`. It does not write `+0x00`–`+0x13`.

**Actor fields read.** Position (three signed 64-bit) at `+0x10`; current action byte `+0x68`; action time (signed 32-bit) `+0x70`; velocity `+0x34`/`+0x38`/`+0x3C`; role byte `+0x60`; kind `+0x64`; flags word `+0x7E`; level `+0x190`; rarity byte `+0x1A8`; class bytes `+0x140`, `+0x141`; health `+0x16C`; block meter `+0x174`; the values `+0x170` and `+0x144`; the timer `+0x12C`; the buff list sentinel `+0x1178`. Buff nodes are doubly linked, next pointer at node `+0x00`, with a type byte at node `+0x08` and a binary32 value at node `+0x0C`. In the target the routine also reads `+0x124`, `+0x5C`, `+0x130`, `+0x184`, `+0x188`, `+0x180` and the worn item slots. In the attacker it also reads the equipment multiplier `+0x17C`.

## 3. Decision order from entry

The steps run in this order. "Return 0" ends the routine with result 0 and no further effect.

**E1. Entry gates.** Return 0 if the target's health is less than or equal to zero (ordered; NaN continues). Return 0 if world byte `+0xB4` is nonzero and the target is world `+0xB8`.

**E2. Attackerless path.** If the attacker is null: one draw, discarded. Then target `+0x1184` is set to 0.5, the working damage R is D, and the routine continues at M6 (section 4). Nothing in E3–M5 runs. The stun step M10 and the action marker M11 are skipped, and the record's attacker key is zero.

**E3. Action gate.** If the attacker's action byte is `0x5B` or `0x4A` and bit 0 of target byte `+0x5C` is clear: return 0, with no draw.

**E4. Attacker effect.** If the attacker's action byte is `0x1C`, one effect record is appended to aggregate list `+0x10` with one draw. Its fields are: the target's raw position at `+0x00`–`+0x17`; zero at `+0x18` and `+0x1C`; 10 at `+0x20`; 0.6, 0.6, 1, 1 at `+0x24`–`+0x33`; at `+0x34`, the scaled unit draw with factor 0.1, plus 0.1; integer 8 at `+0x38`; integer 1 at `+0x3C`; 3 at `+0x40`. The routine continues.

**E5. Miss (record type 3).** It applies when target byte `+0x60` equals 1 and either bit `0x80` of target byte `+0x124` or bit `0x20` of target byte `+0x5C` is set. In order:
1. A presentation record at the target's position: identifier 24, T2 = 1, and T1 = the scaled unit draw with factor 0.3, plus 1 (**one draw**).
2. A hit record, initialised first. Attacker key from attacker `+0x08`; target key from target `+0x08`; damage D; byte `+0x14` = 0; word `+0x18` = 0; target raw position at `+0x20`; V unscaled at `+0x38`; type byte `+0x45` = 3. It is appended to the hit list.
3. The hit routine is called with that record when world byte `+0xB4` is zero. Return 0.

**E6. Block (record type 1).** It applies when the blockable flag is nonzero, the block predicate holds for the target, and target `+0x174` is greater than zero (ordered).

The **block predicate** (`0x044590` / `0x00F610`) fails when the target's signed `+0x130` is positive. Otherwise it holds when the target's action byte is `0x47`, `0x48`, `0x08` or `0x62`, or when the action byte is `0x3B` or `0x3F` and both class bytes `+0x140` and `+0x141` equal 1. Neither this routine nor the predicate examines facing, angle or position.

In order:
1. Q = the block divisor of the target (section 5.3), rounded to binary32. The drain is D divided by Q.
2. Let B0 be the target's `+0x174` at this point. Target `+0x170` becomes `+0x170` plus B0, set to 1 if that sum exceeds 1. Target `+0x144` becomes `+0x144` plus B0, set to the new `+0x170` if it exceeds that.
3. An initialised hit record: keys as in E5, damage D, `+0x14` = 0, `+0x18` = 0, the target position, V times 0.5 componentwise at `+0x38`, type 1.
4. Target `+0x174` becomes B0 minus the drain, set to −1 if it falls below −1.
5. The record is appended to the hit list.
6. A presentation record at the target's position: identifier 24, T2 = 1, T1 = the scaled unit draw with factor 0.3, plus 1 (**one draw**).
7. The hit routine is called when world byte `+0xB4` is zero. Return 0.

**E7. Absorption (record type 5) and continuation.**
1. S is the sum, in list order starting from 0, of the values of the target's type-6 buffs.
2. P is the attacker's action time converted to binary32, divided by the attacker's action duration (section 5.1) converted to binary32. When P exceeds 1, P is replaced by 1; NaN is kept.
3. F = 1 minus P. The absorbed amount is F times S.
4. If the absorbed amount is greater than zero (ordered):
   1. An initialised hit record: keys, damage D (before absorption), `+0x14` = 0, `+0x18` = 0, the target position, V times 0.5, type 5. It is appended to the hit list.
   2. A presentation record at the target's position: identifier 91, T2 = 1, T1 = the scaled unit draw with factor 0.3, plus 1 (**one draw**).
   3. The hit routine is called when world byte `+0xB4` is zero.
   4. R = D minus the absorbed amount. If zero is not less than R (ordered; NaN continues), the routine returns **1** with no main hit.
5. Otherwise R = D.
6. The main hit (section 4) follows.

## 4. Main hit (record type 0)

**M1. Mitigation.** M is the target's resistance (section 5.2) when argument 11 is nonzero, otherwise its armour, rounded to binary32. R becomes R minus (F times M), with F from E7. If the result is below zero (ordered), it becomes positive zero; negative zero and NaN are kept.

**M2. Swing fraction.** P2 is the attacker's action time divided by its action duration, recomputed as in E7 but not capped.

**M3. Per-weapon effects** (only when P2 > 0.25, ordered).
1. A local vector is formed from V. If V's squared length (y² plus x², then plus z²) exceeds zero, x and y are each multiplied by the reciprocal of the square root, which is taken in double precision and rounded to binary32. z is not scaled.
2. The vector becomes (x × 4, y × 4, 5).
3. The attacker's **striking list** (section 5.4) is built.
4. For each listed item in order, the per-weapon effect routine (`0x19A800` / `0x0D2190`) runs. For each extension entry of the item whose type byte (entry `+0x03`, item `+0x17` plus 8 times the index) is `0x80` or more, it appends one effect record to aggregate list `+0x10`. That record holds: the target's raw position; the vector, doubled componentwise when flag B is set; four colour values from routine `0x0C7250` / `0x013DF0` (not audited) given the type byte, (1, 1, 1, 1) and P2 times 0.5; 0.1; an integer count equal to the truncation of P2 times 3 plus 1, raised by 10 when flag B is set; integer 2; and 3.
5. No draws are made. Damage and health are unchanged. The list storage is released.

**M4. Item-extension effects** (only when flag B is set and P2 > 0, ordered).
1. The striking list is rebuilt. If it is empty, nothing happens.
2. Otherwise u = 0.1 divided by the list length, the length converted through double. A heal accumulator H starts at 0.
3. For each item, for each extension entry in index order (count at item `+0x114`; entry i has type byte at item `+0x17 + 8i` and signed 32-bit value at item `+0x18 + 8i`), let c be the level curve of the value converted to binary32:

   | Type | Effect |
   | --- | --- |
   | `0x80` | R becomes ((c × u) × P2) + R |
   | `0x81` | t = (c × u) × P2; R becomes t × 0.5 + R; H becomes t × 2 + H |
   | `0x82` | R becomes (c × (u × 0.25)) × P2 + R; target signed `+0x134` becomes the truncation of (P2 × 500 + `+0x134` as binary32) |
   | `0x83` | R as for `0x82`; attacker signed `+0x138` is updated the same way |
   | other | nothing |

   [Switch table client `0x1986FC`, server `0x0D171C`.]
4. After all items: target `+0x134` is capped at 5000, then attacker `+0x138` is capped at 5000 (signed).
5. If H > 0 (ordered), a hit record is built: initialised; attacker key at both `+0x00` and `+0x08`; the attacker's raw position at `+0x20`; damage = −H; type 0. It is appended to the hit list. Then the attacker's health is increased by H directly, with no clamp. The hit routine is not called for this record.
6. M4 makes no draws, and R is not clamped again after these additions.

**M5. Block bookkeeping and hit presentation.**
1. If the attacker's action byte is `0x1C`: target `+0x1184` becomes 0.3 when 0.3 exceeds it (ordered).
   - Otherwise, if the blockable flag is zero, the routine goes to M6.
   - Otherwise target `+0x1184` becomes 1.
2. Then, only when the blockable flag is set:
   - **Argument 11 nonzero.** A presentation record at the target's position is appended. If attacker class byte `+0x141` is 1, it has identifier 42 and T1 = T2 = 1. Otherwise it has identifier 39, T1 = 1.5 and T2 = 1. No draw.
   - **Argument 11 zero, K not 0x1C.** A presentation record at the target's position with T2 = 1.
     - Selector value: −1 when the striking list is empty, otherwise the subtype byte of its first item; then 17 if the attacker's action byte is `0x44`.
     - **One draw:** T1 = the scaled unit draw with factor 0.2, plus 0.9.
     - Identifier by selector, written "flag A clear / flag A set". Selectors 0 and 1: 1 / 2, and T1 is increased by 0.1. Selector 2: 5 / 6. Selectors 3 and 4: 7 / 8. Selector 5: 3 / 4. Selectors 6, 7 and 8: 9 / 10. Selectors 9 to 14: 5 / 6. Selectors 15 and 16: 1 / 2, with no increase. Selector 17, and any value outside 0 to 17 (−1 included): 5 / 6. [Tables client `0x198708`/`0x19870C` and `0x198738`/`0x19873C`; server `0x0D1748`/`0x0D172C` and `0x0D1778`/`0x0D175C`.]
     - Then routine `0x044230` / `0x00F2B0` is called on the attacker; its result is discarded.
     - Final overrides: the identifier is 11 if the attacker's action byte is `0x36`; otherwise it is 5 if the action byte is `0x0A`.
   - **Argument 11 zero, K = 0x1C.** No record and no draw.

**M6. Velocity check (no effect).** The routine compares the target's squared velocity length with V's squared length and keeps the larger value. It then rescales the target's velocity only if a recomputation of the target's squared length exceeds that maximum. The recomputation uses the same operations in the same order, so the condition cannot hold, and target velocity is never changed here.

**M7. Main record.** The record is not initialised. Its fields:

| Offset | Width | Value |
| --- | --- | --- |
| `+0x00` | 64 | attacker `+0x08`, or zero when the attacker is null |
| `+0x08` | 64 | target `+0x08` |
| `+0x10` | f32 | R, after the type-1 buff step below |
| `+0x14` | 8 | flag A |
| `+0x18` | 32 | 0, or the stun duration from M10 |
| `+0x1C` | 32 | 0 |
| `+0x20` | 3×64 | the target's raw position |
| `+0x38` | 3×f32 | V; z becomes 10 when M10 stuns |
| `+0x44` | 8 | flag B |
| `+0x45` | 8 | type 0 |
| `+0x46` | 8 | 0, or 1 from M11 |
| `+0x47` | 8 | not written |

Before R is stored, it is multiplied in list order by the value of every type-1 buff of the target. No clamp follows.

**M8. Kind 0x11.** If K is 0x11, a 24-byte buff entry is appended to the target's buff list (count `+0x117C`, limit `0x7FFFFFE`). It holds: type byte 4; value R × 0.1; duration 3000; the attacker key at `+0x10`. Bytes `+0x01`–`+0x03` and `+0x0C`–`+0x0F` are not written but are copied. A 40-byte record is also appended to aggregate list `+0x58` (count `+0x5C`, limit `0x5555554`): the attacker key, the target key, then that buff entry. The attacker is dereferenced without a null check, so this path faults on the attackerless path.

**M9. Flag-A presentation.** If flag A is set and target byte `+0x60` is not 6:
1. **One draw:** T1 = the scaled unit draw with factor 0.1, plus 1. T2 = 1. The position is the target's.
2. The identifier is chosen by target kind `+0x64`:

   | Kind | Identifier |
   | --- | --- |
   | 0 | 60 |
   | 1 | 61 |
   | 2, 43 | 62 |
   | 3, 45 | 63 |
   | 4 | 64 |
   | 5 | 65 |
   | 7 | 66 |
   | 8 | 67 |
   | 9 | 68 |
   | 10 | 69 |
   | 11 | 70 |
   | 12 | 71 |
   | 13 | 74 |
   | 14 | 75 |
   | 15 | 72 |
   | 16 | 73 |
   | 37–40 | 79 |
   | 46, 108, 109, 114, 115 | 77 |
   | 48, 51, 87 | 78 |
   | 96 | 80 |
   | 119 | 76 |

   For any other kind, the draw has been made but no record is appended. [Tables client `0x1987C4`/`0x19876C`, server `0x0D17E4`/`0x0D178C`.]

**M10. Stun roll.** It applies only with an attacker, when target signed `+0x12C` is below −3000 and target byte `+0x60` is not 6. W (argument 6) is adjusted in this order:
1. W becomes 1 if the attacker's action byte is `0x3A`, `0x0C`, `0x44`, `0x5D` or `0x45`.
2. W is multiplied by 0.15 if the target's kind is in the **kind set** {101, 107, 108, 109, 114, 115, 116, 117, 118, 119} (`0x0445D0` / `0x00F650`).
3. W is multiplied by 0.1 if the attacker's kind is not in that set and attacker byte `+0x60` is nonzero.
4. W becomes 1 if the attacker's action byte is `0x5B` or `0x4A`.
5. W becomes 1 if the target's action byte is `0x54`.

The chance p is the power of two of ((curve of the attacker's level minus curve of the target's level) × 10), times W. It is set to 1 if it exceeds 1 (ordered), then multiplied by 0.9, and it becomes positive zero if the target has any type-1 buff.

**One draw:** the stun applies when p is greater than the unit draw (ordered). It sets:
- record `+0x18` to the stun duration (section 5.5);
- record z (`+0x40`) to 10;
- one presentation record: identifier 23 at the target's position, T1 = T2 = 1, appended inline.

**M11. Action marker.** With an attacker, record byte `+0x46` becomes 1 if the attacker's action byte is `0x1C`.

**M12. Delivery.** The record is appended to the hit list. The hit routine is called with it when world byte `+0xB4` is zero, and the routine returns 1.

### 4.1 Direct draws in order

| Path | Draws before the hit-routine call |
| --- | --- |
| Attackerless | E2 (1, discarded); M9 (1 when flag A is set and target `+0x60` ≠ 6) |
| Miss | E4 (1 when the attacker action is `0x1C`); E5 (1) |
| Block | E4 (as above); E6 step 6 (1) |
| Absorption | E4; E7 (1); and, when damage remains, the main-hit draws continue after that hit call |
| Main | E4; M5 (1 when argument 11 is zero, K ≠ 0x1C and the blockable flag is set); M9 (1 when gated); M10 (1 when gated) |

No callee of the attack routine draws, apart from the hit routine. The hit routine reaches draws only through its fatal branch, as specified in the earlier contracts. Those draws occur during the hit-routine call, which is the last action of each path.

## 5. Callees

### 5.1 Action duration (`0x044CB0` / `0x00FCF0`)

The result is the attacker's `+0x17C` times a factor, truncated to signed 32 bits. The factor depends on the item in slot `+0xAA8`:
- If its category is 3 and its subtype is 6 to 17: 60 for 6; 80 for 8; 20 for 11, 12, 15, 16 and 17; 50 for 7, 9, 10, 13 and 14.
- If its category is 3 with another subtype: 50.
- Otherwise: 20 when bit `0x10` of `+0x7E` is set, else 50.

[Table client `0x044D2C`, server `0x00FD6C`.]

### 5.2 Armour and resistance (`0x03CFF0` / `0x008300` and `0x0467A0` / `0x011540`)

The two routines are the same except for their multiplier and item routine: armour uses multiplier `+0x184` and item routine `0x0C6A90` / `0x0139B0`; resistance uses `+0x188` and `0x0C7AF0` / `0x014260`.

1. A = the power of two of (3 × curve of the level, computed inline).
2. Br = the power of two of (rarity × 0.25).
3. The base is the multiplier times (Br × A). When byte `+0x60` is 0, the base is instead (2 × A) × the multiplier.
4. When bit `0x20` of `+0x7E` is set, Br × A is added. This uses the rarity factor even when `+0x60` is 0.
5. The item values of slots `+0x530` (category 4), `+0x648` (6), `+0x760` (5) and `+0x878` (7) are added in that order, each slot only when its category matches.

An item value is zero unless the category is 4 to 7. Otherwise it is the power scale of (level plus 0.1 times extension count, rarity), as defined in the earlier contract, times 1 for category 4 or 0.5 for others, times a material factor:
- Armour: 0.8 for material 18; 0.85 for materials 19, 26 and 27; 0.75 for materials 23 and 25.
- Resistance: 0.85 for materials 1 and 19; 0.75 for materials 26 and 27.
- Otherwise the material factor is 1.

### 5.3 Block divisor (`0x03E190` / `0x0094A0`)

Let m be the target's `+0x180` and A as in 5.2. The routine computes m × (A × m + bonus), where the bonus depends on the slots:
- If slot `+0x990` has category 3 and subtype 13, the bonus is 4 times the weapon value of the item in slot `+0xAA8`, whatever that item's category.
- Otherwise, the bonus is the weapon value of `+0x990` when its category is 3, plus the weapon value of `+0xAA8` when its category is 3.

The **weapon value** (`0x0C7F60` / `0x014550`) is zero unless the category is 3. Otherwise it is the power scale of (level plus 0.1 times extension count, rarity), times a subtype factor:
- 2 for subtypes 3, 4 and 13;
- 4 for subtype 5;
- 8 for subtypes 6, 7, 8, 10, 11, 15, 16, 17 and 18;
- 4 otherwise.

### 5.4 Striking list (`0x03DF60` / `0x009270`)

The list of item pointers is built from slots `+0xAA8` and `+0x990`:
1. If `+0xAA8` has category 3 and subtype 6 or 7, the list is that item alone.
2. Otherwise, if `+0x990` has category 3 and subtype 6 or 7, the list is that item alone.
3. Otherwise the list depends on the action byte. In each case a slot is listed only when its category is 3:

   | Action bytes | List |
   | --- | --- |
   | 2, 4, 7, 8, 10, 18, 39, 41 | `+0x990` |
   | 11, 12, 16, 17, 20, 54, 86, 91, 96 | `+0x990`, then `+0xAA8` |
   | 40, 42 | `+0xAA8` |
   | any other | `+0xAA8`, then `+0x990` only if its subtype is 12 |

### 5.5 Stun duration (`0x044FF0` / `0x00FFE0`)

The duration is 1500 when target byte `+0x60` is 0. Otherwise it is 3000 minus the truncation toward zero of (rarity × 1500) divided by 4.

### 5.6 Power routines used by callers

**Heal power** (`0x046E80` / `0x011AD0`). Let m be `+0x180`, A as in 5.2 and Br the rarity factor of 5.2.
1. The base is m × (1 × A).
2. If the striking list is non-empty, the result is the base plus each listed item's weapon value, added in list order. Otherwise:
   - when bit `0x08` of `+0x7E` is set, the base becomes ((A × Br) × 2) plus the base;
   - when bit `0x10` is set, the result is ((A × Br) × 8) plus the base;
   - otherwise the result is the base.

No draws are made.

**Attack power** (`0x03DC60` / `0x008F70`) performs the same steps and then multiplies by an action factor: 50 for action `0x57`, 2 for `0x5B`, 5 for `0x5D`, 3 for `0x60`, otherwise none.

**Chance rolls**, each making **one draw**:
- **Roll Cr** (`0x0444A0` / `0x00F520`). The chance is 1 if the actor has a type-11 buff. Otherwise it is routine `0x03EA40` / `0x009AC0` (not audited) plus `+0x1190` × 0.15. The roll succeeds when the chance is greater than the unit draw.
- **Roll Cw(x)** (`0x0441A0` / `0x00F220`). x is multiplied by 0.5 when the weapon in slot `+0xAA8` (category 3) has subtype 0, 1, 2 or 12, and by 0.3 for subtype 8 or 10. The roll succeeds when x is greater than the unit draw.

## 6. The seven world-update call sites

All seven sites are in the world update routine (client `0x20C510`, server `0x1322D0`), including its unattributed range. Each passes the output aggregate as argument 9 and the routine's changed-zone collection as argument 10. Arguments 7 and 13 are never read by the attack routine. "Draws" counts the direct draws in the window shown before the call. Upstream draws in the same loops were not audited. Context labels marked (lead) come from the reconstruction and were not confirmed.

| Site | Client / server | Context | Target / attacker | Damage D | A | B | W | V | Arg 11 | K | Arg 14 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | `0x20DD18` / `0x133AD8` | zone objects of types 6, 7, 8 (traps, lead) | creature / null | power of two of ((level at the object − 1) × 0.25), times 10; times 5 for type 8 | 0 | 0 | 0 | push P1 | type is 6 | 0 | 1 |
| 2 | `0x20F45D` / `0x13521D` | blast of a creature (bomb, lead) | victim / blast creature | attack power × 25, doubled when A | r mod 4 = 0 | 1 | 0 | push P2 | 1 | 0 | 1 |
| 3 | `0x21476B` / `0x13A52B` | melee strike | struck creature / striker | strike damage S3 | A3 | B3 | striker `+0x13B4` | push P3 | action is `0x1E`–`0x21` | striker action | 1 |
| 4 | `0x2166C8` / `0x13C488` | strike variant (lead: block strike or drain) | creature / striker | S4 | Cr | action is `0x5F` or `0x68` | W4 | V4 | action is `0x5F`, `0x5E` or `0x1C` | 0 | 1 |
| 5 | `0x2183DA` / `0x13E19A` | the creature's type-4 buffs | the creature / buff source (may be null) | buff value | 0 | 0 | 0 | zero | 1 | 0 | 0 |
| 6 | `0x221E8C` / `0x147C4C` | projectile hit, first loop | target / projectile owner | projectile `+0x48`, doubled when A, then raised to +0 if negative | Cr on attacker | projectile byte `+0x5C` | projectile `+0x54` | PP | projectile type is 1 | 0 | 1 |
| 7 | `0x222AB8` / `0x148878` | projectile hit, second loop | target / attacker slot | projectile `+0x48`, doubled when A, no clamp | Cr on owner | projectile byte `+0x5C` | projectile `+0x54` | PP | projectile type is 1 | 0 | 1 |

Notes for each site:

1. **Site 1.** A zone-object record qualifies when its byte `+0x30` is nonzero and either:
   - its type (`+0x00`) is 6 or 7, and two clock values divided by 200 differ (world `+0x8000BC` is one operand); or
   - its type is 8 and its `+0x34` is zero.

   Creatures with byte `+0x60` equal to 1 are skipped. A range test with constant 25 is applied (routines client `0x02C7A0`, `0x03B4A0`, `0x02C860` / server `0x002C50`, `0x006260`, `0x002D10`; not audited). The level comes from routine `0x19A9B0` / `0x0D2340` (not audited). P1 is (creature minus object) in presentation units with z set to 0, normalised when its squared length exceeds 0, then scaled by 20. z is then set to 5, and the vector is doubled for type 8.
2. **Site 2.** The victim must have:
   - health greater than zero (NaN passes);
   - signed `+0x128` equal to 0;
   - no entry in the blast creature's set at `+0x11AC`;
   - a horizontal reach test passed against (0.7 × victim size x + 8) squared (`0x1A7B50` / `0x0DADE0`, comparison not audited);
   - (victim size z × 0.5 + blast size z + 8) greater than the absolute height difference.

   P2 is (victim minus blast) with z = 0, normalised when its squared length exceeds 0.01, then z = 0.25, then scaled by 10. **One draw** decides A, after normalisation. The victim key is inserted into the blast creature's `+0x11AC` set before the call. Upstream gates of this loop were not audited.
3. **Site 3.** A hostility test (`0x1988A0` / `0x0D18C0`) must pass after a geometric test (`0x19EE90` / `0x0D4D80`, not audited); otherwise control goes to the area heal (section 7). If the target's signed `+0x128` is nonzero, the target key goes into the striker's `+0x11B4` set and no attack is made.
   - **A3** is 1 when striker byte `+0x13C0` is nonzero, with no draw. Otherwise it is roll Cr on the striker (**one draw**).
   - **S3.** When the striker's action is `0x57`, `0x58` or `0x1E`–`0x21`, S3 is ((1.25 − scaled unit draw with factor 0.5) × (damage scale × heal power)) × 1. Otherwise S3 is (1.25 − scaled unit draw with factor 0.5 + 0) × ((damage scale × attack power) × (P × P × 5 + bonus)). Each form makes **one draw**, after A3.
     - The damage scale comes from routine `0x03EC90` / `0x009D10` (not audited), given the action.
     - P is striker `+0x13B4`.
     - The bonus is 1, except 0 for striker actions `0x3C`, `0x0B` and `0x3E` (set at `0x139E4F`).
     - S3 is doubled when A3 is set.
   - **B3** is 1 when striker `+0x13B4` is greater than zero or the striker's action is `0x0B` (`0x1393F4`).
   - **P3** is (target minus striker) with z = 0, normalised above 0.01, then z = 0.25. It is scaled first by routine `0x024B70` / `0x0024E0` of a frame ratio, then by routine `0x044860` / `0x00F8F0` of the striker's action (neither audited).
   - Before the call, the target key is inserted into the striker's `+0x11AC` set and striker byte `+0x13B8` is set to 1.
   - The return value gates the post-hit work at `0x13A538`, which was not audited.
4. **Site 4.** A range test and a vertical test (target size z × 0.5 + 1 compared with a height difference) must pass. The target is then inserted into a set, and **A** is roll Cr on the striker (**one draw**).
   - **S4** defaults to (heal power × 1.5) × (scaled unit draw with factor 0.05, plus 1); that is **one draw**. For striker action `0x68`, S4 is replaced by (1.25 − scaled unit draw with factor 0.5 + 0) × ((damage scale × attack power) × (P × P × 10)), with **one more draw**. S4 is doubled when A is set.
   - The hostility test must then pass.
   - **W4** is 0, 0.1 for action `0x5F`, and P for action `0x68`.
   - **V4** is zero, except for action `0x68`: a vector from routines `0x027870` and `0x0252F0` (server `0x012670`, `0x0079F0`; not audited) with factor 10, with z set to 5.
5. **Site 5.** For each type-4 buff entry of the creature, when world byte `+0xB4` is zero and a clock value divided by 400 has changed: the source is looked up from the buff's key at entry `+0x10` (`0x02F000` / `0x005420`), and the attack routine is called. These buffs are the ones M8 creates. Type-1 entries take a different branch, which zeroes the creature's `+0x12C` when it is positive.
6. **Site 6.** The loop works on one projectile record at a time. The owner and attacker are both found by looking up the projectile's key at `+0x00` (`0x147236`, `0x147246`, `0x14732C`).
   - Before A, when the projectile byte `+0x5C` is zero, its type `+0x60` is not 1, 3 or 4, and the attacker exists, there are **two draws**. The first gives a gain of ((1 − scaled unit draw with factor 2) × 0.05 + 0.1); this is multiplied by 0.2 for type 2, and then multiplied by the result of routine `0x03D1A0` / `0x0084B0` divided by 300. The second draw doubles the gain when it mod 8 is zero. The gain is added to attacker `+0x170` with a cap of 1, when world byte `+0xB4` is zero or the attacker is world `+0xB8`.
   - Then A (Cr, **one draw**, only when the attacker exists).
   - Then roll Cw(0.15) on the owner (**one draw**). This is gated on the owner being world `+0xB8`, or world byte `+0xB4` being zero with owner byte `+0x60` nonzero.
   - If projectile byte `+0x64` is 2, the heal of section 7 replaces the attack call.
   - PP is formed at `0x1472A0`: the projectile vector at a local, normalised when its squared length exceeds 0, scaled by `+0x54` × 5, with z = `+0x54` × 3.
   - The return value is merged into a local hit flag.
7. **Site 7.** If the target's signed `+0x128` is nonzero and the attacker exists, the target key goes into the attacker's `+0x11B4` set, and no attack is made.
   - Then A (Cr on the owner when it exists, **one draw**). D is the projectile's `+0x48`, doubled when A is set; it is not clamped.
   - Then Cw(0.25) on the owner, gated as at site 6 (**one draw**).
   - Where this loop loads the attacker argument was not traced.
   - The return value is merged into a hit flag.

## 7. Heal amounts passed directly to the hit routine

Each heal record is built by the hit-record initialiser, so its type is 0 and its stun fields are zero. It is appended to the hit list and then passed to the hit routine when world byte `+0xB4` is zero, or when both the acting creature and the target are world `+0xB8`.

**Area heal** (`0x214BAC` / `0x13A96C`). It is reached from site 3's strike loop when the hostility test fails, the striker's action is `0x20` or `0x21`, a local flag at frame slot `−0x2C70` is set, and striker byte `+0x141` is 1.
1. **One draw:** roll = 1.25 minus the scaled unit draw with factor 0.5.
2. Damage = ((heal power of the striker × roll) × 1) × the binary32 at `0x302AC8` / `0x173CD0` (`0xBDCCCCCD`, −0.1).
3. Byte `+0x14` = roll Cr on the striker (**one draw**, made after the damage is computed; it does not change the damage).
4. Keys: the striker and the target. Position: the target's.
5. A presentation record: identifier 42, T1 = T2 = 1, at the striker's position.

**Beam heal** (`0x214F72` / `0x13AD32`). The healer is the acting creature. The target is the creature found from the healer's key at `+0x1A0`, or the healer itself when that lookup fails. The path is gated on:
- the world flag and role tests at `0x13AAAB`;
- interval timing on the healer's `+0x6C` (server routines `0x007DB0`, `0x011D60`, factor 0.125);
- the healer's `+0x170` being at least the cost from server routine `0x00FB20` (ordered). That cost is then subtracted from `+0x170`.

In order:
1. **One draw:** roll = 1.25 minus the scaled unit draw with factor 0.5.
2. Damage = (heal power of the healer × −2) × roll.
3. Byte `+0x14` = Cr on the healer (**one draw**).
4. Keys: the healer and the target. Position: the target's.
5. A presentation record: identifier 41, T1 = T2 = 1, at the healer's position.

**Projectile heal** (`0x221E14` / `0x147BD4`). Taken at site 6 when projectile byte `+0x64` is 2. It requires the owner to exist, the enemy test (`0x196CA0` / `0x0CFCC0`) of owner and target to be false, and target byte `+0x60` not 6.
- Damage = minus D, where D is the site-6 amount after doubling and the clamp. It is multiplied by 0.5 when the owner's byte `+0x60` is 1.
- Byte `+0x14` = A.
- Record `+0x00` takes the projectile's 64-bit `+0x00`, `+0x08` the target key.
- The position is copied from a frame pointer that was not traced.
- If projectile byte `+0x64` is 2 but the owner test fails, neither a heal nor an attack is made.

**Sign of heal damage.** The roll lies between 0.75 and 1.25 and is positive. A and the rarity factor are never negative, and weapon values are non-negative for finite inputs. So:
- **Heal power** is non-negative unless the multiplier `+0x180` is negative. It is zero when `+0x180` is zero, or when the level makes A zero (exactly −19, as in the earlier contract) with no weapon bonus. It is NaN for NaN inputs.
- **Area and beam heals** are negative whenever heal power is positive. They are negative zero when heal power is positive zero. They are positive only when heal power is negative.
- **The projectile heal** is never positive and finite. It is negative for positive D, positive zero when the projectile value is negative zero (the clamp keeps negative zero, and negation gives positive zero), negative zero when D is positive zero, and NaN when D is NaN.
- In the hit routine, a zero heal takes the non-damage path.

## 8. Other effects of the attack routine

The attack routine writes these target fields: `+0x1184` (E2, M5), `+0x170`, `+0x144` and `+0x174` (E6), and `+0x134` (M4). It writes these attacker fields: `+0x138` and health (M4). It does not write the target's health; health changes come only through the hit routine. Its records go to aggregate lists `+0x00`, `+0x08`, `+0x10` and `+0x58`. The type-4 buff entry goes to the target's list at `+0x1178`.

## 9. Client and server

The following agree instruction by instruction in the two images, apart from absolute addresses, which were all paired and resolved:
- the whole attack routine, including the two client listing omissions at `0x1976B3`–`0x1976D8` and `0x197AA6`–`0x197AC1`, read from the client image;
- all callees named in sections 2 and 5, including the omitted tails of the heal-power and attack-power routines, read from the image;
- the windows before all seven call sites and the three heal sites;
- the site-3 helper windows at `0x1393A1` and `0x139E44`.

All jump tables agree in relative targets and index bytes. Every constant agrees in value: 0, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.5, 0.7, 0.75, 0.8, 0.85, 0.9, 1, 1.25, 1.5, 2, 3, 4, 5, 8, 10, 20, 25, 50, 60, 80, 300, 500, 32767, −1, −2, −0.1, 0.125, 2^−16, the sign mask, the double constants 1, 2 and 2^32, and the material, subtype and action tables.

No behavioural difference exists in these instructions. Runtime values that steer them, such as world byte `+0xB4` and pointer `+0xB8`, are not established for either program.

## 10. Unresolved

- The game meaning of every field, flag, action byte, kind, record identifier and list. "Miss", "block", "absorption", "stun", "critical", "trap", "bomb" and "heal" are descriptive labels only.
- Routines named but not audited beyond their draws and recursion (server RVAs): the base chance `0x009AC0`; the damage scale `0x009D10`; the strength `0x00F8F0`; the frame-ratio scale `0x0024E0`; the colour routine `0x013DF0`; the hostility, geometric, range and lookup tests (`0x0D18C0`, `0x0D4D80`, `0x0DADE0`, `0x005420`, `0x130540`/`0x130600`/`0x130690`); the level-at-position routine `0x0D2340`; the duration helper `0x0084B0`; the interval helpers `0x007DB0`/`0x011D60`; the cost routine `0x00FB20`; the vector routines `0x012670`/`0x0079F0`. Also unaudited: the hit routine's use of record bytes `+0x14`, `+0x44` and `+0x46`.
- Caller-side values not traced to their origin:
  - site 3's frame ratio for P3;
  - the local flag gating the area heal;
  - the source of site 7's attacker argument;
  - the projectile-heal position pointer;
  - the clock operands at sites 1 and 5;
  - the upstream gates of every world-update loop, and any draws made there before the windows shown;
  - the post-hit work after sites 3 and 4 that their return value gates.
- Whether the attackerless path is ever reached with kind 0x11. It would fault in M8.
- Whether a running program can have a negative or zero `+0x180`, or level −19. These make heal power zero or negative, and so make the area and beam heals zero or positive.
- The size and remaining fields of the effect records at aggregate `+0x10`, and how the aggregate lists are consumed.
- Floating-point control state, exceptions, and the unwritten bytes noted above (record `+0x47`, buff entry bytes `+0x01`–`+0x03` and `+0x0C`–`+0x0F`).
