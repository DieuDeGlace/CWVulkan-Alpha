# Alpha critical and haste stat numerics

Status: FINAL / STABLE / HELD after complete prose readback by the author.

## Supported image and scope

The supported original image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. All evidence paths below are relative to the original `disassembled alpha cube world` root. Addresses are RVAs unless explicitly marked VAs.

This contract recovers the critical-stat query at RVA `0x03EA40`, its item-stat dependency at `0x0C6BA0`, the haste/item-stat query at `0x0478B0`, its item dependency at `0x0C7C00`, and their normalized level/rank power dependency at `0x045FF0`. It closes the arithmetic and field/gate behavior of those routines. It does not recover the higher-level critical-decision or complete attack-speed calculation that consumes their results.

The independently checked original evidence is `decompiled/client/assembly.txt`; original function files `decompiled/client/functions/0043ea40.c`, `004478b0.c`, `004c6ba0.c`, `004c7c00.c`, and `00445ff0.c`; `decompiled/client/external_functions.csv`; and the original binary constants. The assembly establishes call inputs, rounding boundaries and comparison behavior where the decompilations omit or simplify those facts. No other contracts, replacement source, history, probes, tests, captures, SDKs or reconstruction material were consulted. Prior held artifacts remained unopened and unchanged.

## Shared conventions and exact constants

Both creature queries receive a creature pointer in ECX. Both item queries receive an item pointer in ECX. Each returns a stored binary32 value loaded onto the x87 return stack. No scoped routine accumulates its result in extended precision across the documented arithmetic operations.

Every arithmetic operation described as binary32 produces a binary32 result before the next operation. Widening a completed binary32 exponent to binary64 occurs only at the imported power boundary. Each imported binary64 result is narrowed before subsequent game-owned products or divisions.

| Constant use / decimal label | Format | Original bits | Constant RVA |
| --- | --- | --- | --- |
| Fixed initial rank input, positive zero | Binary32 | `0x00000000` | `0x345D84` |
| Arithmetic one | Binary32 | `0x3F800000` | `0x345DC0` |
| Level coefficient / lower critical-item coefficient, 0.05 | Binary32 | `0x3D4CCCCD` | `0x2FCDB4` |
| Base-stat scale / higher critical-item coefficient / lower haste-item coefficient, 0.1 | Binary32 | `0x3DCCCCCD` | `0x2FCB7C` |
| Higher haste-item coefficient, 0.2 | Binary32 | `0x3E4CCCCD` | `0x2FCDBC` |
| Level-exponent scale, 3 | Binary32 | `0x40400000` | `0x2FCD70` |
| Rank-exponent coefficient, 0.25 | Binary32 | `0x3E800000` | `0x345D98` |
| Seed-remainder divisor, 20 | Binary32 | `0x41A00000` | `0x2FCD74` |
| Small-item-result threshold, 0.001 | Binary32 | `0x3A83126F` | `0x2FD870` |
| Imported power base, 2 | Binary64 | `0x4000000000000000` | `0x345E18` |
| Fixed normalization exponent, 3 | Binary64 | `0x4008000000000000` | `0x2FCB98` |
| Remainder conversion's selected correction, positive zero | Binary64 | `0x0000000000000000` | `0x345F30` |

These values were read directly from `binary/Cube.exe` using its PE section mappings. Decimal labels such as 0.05, 0.1, 0.2 and 0.001 refer to the listed binary32 values, not exact decimal real numbers. The normalized denominator is obtained through an actual imported power call using binary64 base two and exponent three; the native code does not substitute a literal eight for that call.

## Initial creature contribution: level, rank and role

The two creature queries have the same initial numeric construction. They read signed 32-bit creature level from `+0x190` and convert it directly to binary32. Starting with that converted level, the native arithmetic subtracts one, multiplies the rounded difference by 0.05, and adds one. A binary32 one is divided by that rounded sum. The quotient is subtracted from binary32 one, and the difference is multiplied by binary32 three. Each subtraction, multiplication, addition and division has its own binary32 boundary.

The completed binary32 level exponent is widened to binary64 and supplied to the first imported power call with binary64 two as the base. The returned binary64 level factor is immediately narrowed to binary32 and saved.

For the second imported power call, the native caller multiplies stored binary32 0.25 by stored binary32 positive zero. That completed binary32 exponent is widened to binary64, again with binary64 two as the base. Its result is narrowed to binary32. The saved level factor is multiplied by that narrowed fixed-rank factor in binary32, and the product is stored before the third imported call.

The third imported call uses binary64 two and the fixed binary64 exponent three. Its result is narrowed to binary32. The saved level/rank product is divided by that narrowed normalization factor in binary32, and that quotient is then multiplied by binary32 0.1. This final rounded value is the initial creature contribution before equipment.

Neither query reads actual creature rank at `+0x1A8` or numeric role at `+0x60` to construct this term. There is no role-zero replacement, nonzero-role rank branch, class/specialization factor, stored health/damage coefficient, combo input or status-list multiplier inside these two routines. Their initial rank exponent is fixed at zero for every creature. All three imported calls still occur before equipment, including the zero-exponent and fixed-normalization calls. No level floor, level ceiling or integer power quantization is applied. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0043EA40–0x0043EB0C` and `0x004478B0–0x0044797C`.]

## Eligible creature slots and sequential additions

Both queries examine exactly the following records in the same order. The critical query invokes RVA `0x0C6BA0` for an accepted slot; the haste query invokes RVA `0x0C7C00`.

| Addition order | Creature-relative record start | Required category byte at that record start |
| --- | --- | --- |
| 1 | `+0x990` | 3 |
| 2 | `+0xAA8` | 3 |
| 3 | `+0x530` | 4 |
| 4 | `+0x648` | 6 |
| 5 | `+0x760` | 5 |
| 6 | `+0x878` | 7 |
| 7 | `+0x418` | 8 |
| 8 | `+0xBC0` | 9 |
| 9 | `+0xCD8` | 9 |

A category mismatch skips that record entirely, even if the item's category would be accepted by the item helper in isolation. No other creature equipment record contributes here. Both category-3 records and both category-9 records can independently contribute. These outer gates impose no item-level, rank, material, subtype, class-compatibility or quantity check.

Each helper return is stored/popped as binary32, loaded as the first operand of a scalar addition with the previous total as the second operand, and added in binary32. That rounded sum becomes the total used for the next accepted record. Even a helper return of positive zero goes through the addition on an accepted slot. The final return loads the stored binary32 total onto the x87 stack. Neither creature query caps the final stat at one, filters the initial contribution through the item threshold, or applies a terminal minimum/maximum clamp. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0043EB11–0x0043EC89` and `0x00447981–0x00447AF9`.]

## Item inputs, category gates and raw fields

Both item helpers accept categories 3, 4, 5, 6, 7, 8 and 9. A different category returns positive zero immediately via the native x87 zero constant, without reading the seed, rank, material or level and without calling the normalized power helper.

For an accepted category, the scoped inputs are:

| Item-relative field | Native read and numeric role |
| --- | --- |
| `+0x00` | One-byte category for eligibility and coefficient selection |
| `+0x01` | One-byte subtype, consulted for coefficient selection only when category is 3 |
| `+0x04` | Raw unsigned 32-bit seed for remainder variation |
| `+0x0C` | Unsigned rank/rarity byte, zero-extended for the power helper |
| `+0x0D` | Material byte, compared for equality with one particular identifier |
| `+0x10` | Raw signed 16-bit item level, sign-extended and converted to binary32 |

Neither item helper reads the extra-block count at `+0x114` or adds a count-derived fractional level. No count field gates an otherwise accepted record. Negative raw item levels remain signed values in the calculation; no zero/one minimum or receiving-creature-level adaptation is applied. The rank byte is not limited to a conventional rarity range before it is passed to power. [Evidence: the full original instruction bodies at VAs `0x004C6BA0–0x004C6CBD` and `0x004C7C00–0x004C7D12` in `decompiled/client/assembly.txt`.]

## Item coefficients and seed/material factors

The higher-coefficient condition is the same in both item helpers: category 4 always selects it; category 3 selects it only for subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18. Every other accepted category/subtype uses the lower coefficient. An unlisted category-3 subtype still receives the lower coefficient rather than being rejected.

| Item helper | Lower coefficient | Higher coefficient |
| --- | --- | --- |
| Critical, RVA `0x0C6BA0` | 0.05 | 0.1 |
| Haste, RVA `0x0C7C00` | 0.1 | 0.2 |

The raw unsigned 32-bit seed is divided by integer 21 and its unsigned remainder is retained. There is no left shift, signed remainder or PRNG draw in this calculation. The remainder lies from 0 through 20. The native conversion first converts that integer remainder to binary64, adds the positive-zero correction selected from the conversion table, and narrows to binary32. The correction-table index is zero for this remainder range. That binary32 remainder is divided by binary32 twenty.

For critical item stats, the resulting binary32 quotient is subtracted from binary32 one. Material identifier 11 then adds binary32 one to that completed difference. Other material identifiers add nothing.

For haste item stats, the quotient itself is the factor. Material identifier 12 adds binary32 one to that quotient. Other material identifiers add nothing. The optional material increment is one further binary32 addition in each case. The critical and haste seed factors therefore use different sides of the remainder variation and different material identifiers. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004C6BD1–0x004C6C71` and `0x004C7C31–0x004C7CC6`.]

## Normalized item power and ordered products

RVA `0x045FF0` consumes a binary32 level and a signed 32-bit rank. In these two item paths, the level is the converted signed 16-bit item field and the rank is the zero-extended unsigned byte, so the rank supplied lies from 0 through 255.

The helper constructs the level exponent with the same binary32 sequence described for the creature term: subtract one from level; multiply by 0.05; add one; divide one by that sum; subtract the quotient from one; multiply the difference by three. The completed exponent is widened for a binary64 power of two. The result is narrowed to binary32 and saved.

For the second power call, the signed rank argument is converted to binary32 and multiplied by binary32 0.25. That rounded exponent is widened to binary64 for a power of two. Its result is narrowed to binary32. The saved level factor is multiplied by the narrowed rank factor in binary32. The product is saved before the third power call, whose base is binary64 two and whose exponent is fixed binary64 three. Its result is narrowed to binary32; the saved product is divided by it in binary32. That stored binary32 quotient is returned on the x87 stack. The helper applies no extra 0.1 scale: that scale belongs to the outer creature initial term or the selected item coefficient.

Each item helper stores/pops this normalized-power return as binary32. It multiplies that value first by its selected coefficient, then multiplies the rounded product by its completed seed/material factor. The two products have separate binary32 boundaries. This is the candidate item stat before the small-result filter. Even when the seed/material factor is zero, the helper obtains all three power results before multiplying and filtering; it has no zero-factor shortcut. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00445FF0–0x0044609C`, `0x004C6C71–0x004C6CAC` and `0x004C7CC6–0x004C7D01`.]

## Small-result comparison and exceptional values

The filter in each item helper compares the stored binary32 threshold 0.001 as its first comparison operand against the binary32 candidate as its second. It returns positive zero only when the threshold is ordered and strictly greater than the candidate. An exactly equal candidate is returned unchanged. An ordered larger candidate is returned unchanged; there is no upper cap.

For a NaN candidate, an unordered comparison does not take the ordered-above branch that returns zero. If native execution reaches and completes that comparison, the stored candidate is loaded for return rather than replaced with zero. NaN quieting and payload behavior across the return/conversion boundary remain subject to the runtime and floating-point controls. Consequently the filter cannot be replaced by an unconditional requirement that the candidate be ordered and at least the threshold. The decompiled condition loses this NaN distinction; the original comparison and branch establish it. Finite negative candidates, negative infinity, and either signed zero are ordered below the positive threshold and return positive zero. Positive infinity passes the filter. Unmasked floating-point exceptions may interrupt these cases before a normal return.

The filter is applied separately to every accepted item's completed candidate before accumulation. It is not a threshold on the sum of several small candidates. Neither creature query supplies an additional NaN rejection after an item return. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004C6C99–0x004C6CBD` and `0x004C7CEE–0x004C7D12`.]

## Imported-power and return boundaries

Every scoped exponentiation goes through the original client thunk at RVA `0x28D8D4`, which jumps through the import slot for `MSVCR110.DLL::_libm_sse2_pow_precise`. The caller supplies the low binary64 half of XMM0 as base and the low binary64 half of XMM1 as exponent; it consumes the returned low binary64 half of XMM0. The prepared upper binary64 halves of both input registers are positive zero in these callers. These import calls do not obtain their result from the x87 stack; the game-owned routines create x87 return values later by loading stored binary32 values.

| Caller | Imported call RVAs, in execution order | Binary64-result narrowing RVAs |
| --- | --- | --- |
| Critical creature query | `0x03EA8A`, `0x03EAB6`, `0x03EAE0` | `0x03EA92` into cleared XMM1; `0x03EABE` into cleared XMM1; `0x03EAF1` in XMM0 |
| Haste creature query | `0x0478FA`, `0x047926`, `0x047950` | `0x047902` into cleared XMM1; `0x04792E` into cleared XMM1; `0x047961` in XMM0 |
| Normalized item power | `0x04602E`, `0x046057`, `0x046081` | `0x04603B` in XMM0; `0x04605F` into cleared XMM1; `0x04608B` in XMM0 |

The item helpers additionally store/pop the normalized helper's x87 return to binary32 at critical RVA `0x0C6C8C` or haste RVA `0x0C7CE1` before their products. Their creature callers similarly store/pop every item-helper return before its binary32 addition. Under a normal completed call, a creature query therefore performs three imported powers for its initial term and three more for each accepted slot. With all nine slots accepted, it performs 30 such calls. A direct item-helper call rejected by category performs none. There is no result cache or random draw in these routines. [Evidence: `decompiled/client/assembly.txt`, the complete scoped bodies and VA `0x0068D8D4`; `decompiled/client/external_functions.csv`, original power import entry.]

## Mutation and remaining limits

The scoped game-owned routines read their supplied creature/item fields and constants without mutating those records, item seeds, counts or game RNG state. Item seed variation is deterministic unsigned arithmetic. No role/class/status mutation, timer, resource payment or critical-decision draw belongs to these stat queries.

This contract closes the original field roles, raw signed/unsigned reads, fixed rank term, nine slot gates and addition order, coefficient and material selections, seed remainder conversion, normalized power arithmetic, product/filter order and binary32/binary64 boundaries. The imported runtime's power algorithm, last-bit results, special-value handling, exception/state effects and full register-preservation behavior were not inspected. Nor was one ambient MXCSR/x87 control state established for every caller. Division by zero at unusual raw levels, overflow/underflow, denormal handling, NaN propagation, exception flags and traps retain those imported-math and ambient-control limits. The documented ordered/unordered filter behavior applies when native execution returns normally.

The higher-level use of these stat results, including extra critical-threshold inputs, status-based critical decisions, combo/class speed terms, timing and final speed multipliers, remains outside this bounded recovery. No replacement formula or clamp is supplied for those separate behaviors or for unresolved runtime limits.
