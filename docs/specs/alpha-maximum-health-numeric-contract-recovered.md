# Alpha maximum-health numeric contract — independently recovered

Status: FINAL / STABLE / HELD after the complete prose readback reported by the author. This is a bounded original-evidence contract for the maximum-health query and its numeric dependencies. It does not certify a complete health, combat, progression or replacement-engine system.

## Supported image and evidence

The supported original image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred image base `0x00400000`. All evidence paths in this document are relative to the original evidence root, `disassembled alpha cube world`. Routine identifiers below are RVAs; an instruction VA is identified explicitly when used to locate evidence.

The primary evidence is the original image together with `decompiled/client/assembly.txt`, specifically maximum health at RVA `0x044DB0`, fixed item health at RVA `0x0C70B0`, continuous level/rank power at RVA `0x045F60`, and the imported power thunk at RVA `0x28D8D4`. The corresponding original decompilations are `decompiled/client/functions/00444db0.c`, `004c70b0.c` and `00445f60.c`; their recovered call signatures omit register arguments to the imported math routine, so the original instructions establish the numeric contract. `decompiled/client/external_functions.csv` identifies the import as `MSVCR110.DLL::_libm_sse2_pow_precise`.

This recovery used original evidence only. No earlier maximum-health contract, reconstruction, replacement-engine source, test, capture, probe output or implementation handoff was consulted.

## Query inputs, result and field roles

RVA `0x044DB0` receives one creature record and returns the maximum-health value as a binary32 value carried on the x87 return stack. The final return loads a stored binary32 value; there is no extra extended-precision health accumulation. The query does not read current health at creature offset `+0x16C` and does not write health or any creature/item field.

| Creature-relative field | Native read width and interpretation | Role in this query |
| --- | --- | --- |
| `+0x060` | One byte, compared for equality with zero | Numeric role discriminator selecting the fixed role-zero rank factor |
| `+0x140` | One byte, treated as an unsigned class identifier for dispatch | Class scaling, with specific cases 1, 2 and 4 |
| `+0x141` | One byte, equality comparison | An additional specialization factor only when both class and specialization are 1 |
| `+0x178` | Binary32 | Stored creature health coefficient, multiplied directly into the creature contribution |
| `+0x190` | Signed 32-bit integer converted to binary32 | Creature level |
| `+0x1A8` | Unsigned byte, zero-extended before conversion to binary32 | Creature rank used by the initial rank power call |
| Equipment records in the table below | Each begins with a one-byte category | Potential fixed item-health additions |

The role byte, class byte and specialization byte are distinct inputs. The role decision occurs before class and specialization scaling. Class scaling and the class-1/specialization-1 factor remain active for role zero. Every nonzero role takes the same rank-dependent branch in this query; no distinction among nonzero role values is made here. The numeric identifiers and offsets are authoritative; this contract does not assign names to all role values. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444DC4–0x00444ED5`.]

The stored health coefficient is not clamped, replaced with a universal base-health constant, or multiplied by the class factor after equipment has been added. Its existing binary32 value is the input. This scope does not specify every writer that establishes that coefficient during creature creation or subsequent state changes.

## Exact constants

The following are the stored constants used by the bounded path. Decimal labels identify their intended magnitudes; the bit patterns specify the values actually supplied by this image. In particular, the labels 0.05, 0.1, 1.1, 1.2 and 1.3 denote the listed binary32 values, not exact decimal real numbers.

| Use / decimal label | Format | Bit pattern | Constant RVA |
| --- | --- | --- | --- |
| Arithmetic one | Binary32 | `0x3F800000` | `0x345DC0` |
| Level coefficient, 0.05 | Binary32 | `0x3D4CCCCD` | `0x2FCDB4` |
| Level exponent scale, 3 | Binary32 | `0x40400000` | `0x2FCD70` |
| Power base, 2 | Binary64 | `0x4000000000000000` | `0x345E18` |
| Rank exponent coefficient, 0.25 | Binary32 | `0x3E800000` | `0x345D98` |
| Role-zero exponent, 1 | Binary64 | `0x3FF0000000000000` | `0x345DF8` |
| Class 1 factor, 1.3 | Binary32 | `0x3FA66666` | `0x2FD8E4` |
| Class 2 factor, 1.1 | Binary32 | `0x3F8CCCCD` | `0x2FD780` |
| Class 4 factor, 1.2 | Binary32 | `0x3F99999A` | `0x2FCDCC` |
| Class 1 / specialization 1 factor, 1.25 | Binary32 | `0x3FA00000` | `0x2FCDD0` |
| Non-chest item category factor / material-26 increment, 0.5 | Binary32 | `0x3F000000` | `0x345DA0` |
| Seed-remainder divisor, 20 | Binary32 | `0x41A00000` | `0x2FCD74` |
| Material-27 increment, 0.75 | Binary32 | `0x3F400000` | `0x2FCDC4` |
| Extra-block count coefficient, 0.1 | Binary32 | `0x3DCCCCCD` | `0x2FCB7C` |
| Fixed item-health scale, 5 | Binary32 | `0x40A00000` | `0x2FCE24` |
| Unsigned-conversion table entry actually selected for the remainder | Binary64 | `0x0000000000000000` | `0x345F30` |

These values were read directly from `binary/Cube.exe` after resolving its PE section mappings. The original instructions at the three scoped routines identify the operand widths and constant addresses. The class dispatch table at RVA `0x044FE0` selects factors for classes 1, 2 and 4 and leaves class 3 unchanged. [Evidence: `binary/Cube.exe`, constant RVAs in the table and dispatch table RVA `0x044FE0`; `decompiled/client/assembly.txt`, VAs `0x00444E7F–0x00444ED5`.]

## Continuous level/rank power

RVA `0x045F60` consumes a binary32 level input and a signed 32-bit rank input. In the fixed item-health path, the caller has already widened the item rank from an unsigned byte, so that rank lies between 0 and 255. There is no rank clamp in this power routine.

The level exponent is constructed in binary32. Starting from the supplied level, the native arithmetic subtracts one, multiplies that rounded difference by the stored 0.05 coefficient, and adds one. A binary32 one is divided by that rounded sum. The quotient is subtracted from a binary32 one, and the resulting difference is multiplied by binary32 three. Every subtraction, multiplication, addition and division in this paragraph produces a binary32 result before the next operation. There is no integer conversion or truncation of this exponent.

That completed binary32 exponent is widened to binary64 and supplied as the exponent of the first imported power call, with binary64 two as the base. The imported binary64 result is converted to binary32 immediately. The rank integer is separately converted to binary32, multiplied by binary32 0.25, and widened to binary64 for the second imported power call, again with binary64 two as the base. Its binary64 result is immediately converted to binary32. The saved level factor is then multiplied by the rank factor in binary32. That stored binary32 product is loaded onto the x87 return stack. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00445F60–0x00445FE5`.]

The maximum-health routine uses this same level-exponent arithmetic directly for the creature's signed 32-bit level, including an initial signed-integer-to-binary32 conversion. Its rank input is the unsigned byte at `+0x1A8`. The creature path calculates its own powers directly rather than calling RVA `0x045F60`. Neither health path obtains its continuous multiplier from the separate integer-valued level/power query at RVA `0x045F10`. There is no experience read, level floor, level ceiling, adaptation to a receiving creature's level, or integer power quantization in the bounded health path. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444DC4–0x00444E29`, `0x00445F10–0x00445FE5` and `0x004C7159–0x004C718B`.]

## Creature contribution and role/class precedence

The first imported power call calculates the creature level factor using the level exponent just described. Its result is narrowed to binary32 and retained. The second imported power call uses the zero-extended creature rank multiplied by binary32 0.25 as its exponent. Its narrowed binary32 result is multiplied first by the retained level factor and then by the stored health coefficient at `+0x178`, with a binary32 result at each multiplication.

The role comparison follows those two imported power calls. For any nonzero role, their completed product is retained. For role zero, a third imported power call supplies binary64 two raised to the fixed binary64 exponent one. That result is narrowed to binary32, multiplied first by the retained level factor and then by the same stored health coefficient, again with a binary32 boundary at each product. This replaces the preliminary rank-dependent creature contribution. Consequently, role zero still evaluates the creature-rank power and preliminary products before the role-zero replacement; their imported-call and floating-point-status effects are part of the observed ordering even though their numeric product is superseded. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444DFA–0x00444E7A`.]

The selected creature contribution is then multiplied in binary32 by the class factor: class 1 uses 1.3, class 2 uses 1.1, and class 4 uses 1.2, with the exact bits listed above. Every other class byte receives no multiplication at this stage. After that class operation, class 1 with specialization 1 receives a further binary32 multiplication by 1.25. There is no corresponding specialization multiplication for any other class or specialization value. Equipment health is added only after these operations, so these class factors apply to the creature contribution rather than to the equipment additions. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444E7F–0x00444EDA`; `binary/Cube.exe`, dispatch table RVA `0x044FE0`.]

## Equipment eligibility and accumulation order

The native query examines exactly these six fixed records, in the following order. Each accepted record is passed to RVA `0x0C70B0`; its returned binary32 item-health value is added to the current binary32 total before the next record is considered.

| Addition order | Creature-relative record start | Required category byte at the record start |
| --- | --- | --- |
| 1 | `+0x990` | 3 |
| 2 | `+0xAA8` | 3 |
| 3 | `+0x530` | 4, native category name `ChestArmor` |
| 4 | `+0x648` | 6, native category name `Boots` |
| 5 | `+0x760` | 5, native category name `Gloves` |
| 6 | `+0x878` | 7, native category name `ShoulderArmor` |

A record whose category does not match its particular gate contributes nothing, even if that category would be accepted by the item helper on its own. No other equipment record contributes in this query. There is no subtype, class-compatibility, material, item-level, identification, count-of-equipped-items or receiving-creature-level gate before these six calls. Both category-3 records may independently contribute. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444EDA–0x00444FD8`. Category names are established by original dictionary assignments at VAs `0x005901BA–0x00590251` in the same file, with the matching entries in `decompiled/client/strings.csv`.]

At each accepted addition, the helper's x87 result is stored as binary32, then loaded as the first scalar addition operand; the previous total is the second operand. The binary32 sum becomes the total for the following slot. The final returned total is a stored binary32 value. The query performs no terminal minimum, maximum, nonnegative clamp or saturation. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444EE9–0x00444FDE`.]

## Fixed item-health inputs and category rejection

RVA `0x0C70B0` accepts one item record. An item category other than 3, 4, 5, 6 or 7 returns positive zero immediately, through the x87 zero constant. On that rejected path it does not read the seed, rank, material, item level or extra-block count and does not call the power dependency.

For an accepted category, the following fields are consumed. Other item fields, including subtype at `+0x01`, are not consulted by this helper.

| Item-relative field | Native interpretation | Numeric role |
| --- | --- | --- |
| `+0x00` | One-byte category | Eligibility and the category factor |
| `+0x04` | Raw 32-bit word | Deterministic seed variation after a wrapping left shift |
| `+0x0C` | Unsigned byte | Item rank/rarity argument for continuous power |
| `+0x0D` | Unsigned byte | Material identifier selecting a seed-factor increment |
| `+0x10` | Signed 16-bit integer, sign-extended | Raw item level component |
| `+0x114` | Signed 32-bit integer | Extra-block count contributing a fractional level component |

The count role at `+0x114` is also supported by native item-record insertion at RVA `0x0C69B0`, which uses it as the number of eight-byte records beginning at item offset `+0x14`. That writer's ordinary size restrictions are separate from this health helper: RVA `0x0C70B0` reads and uses the entire signed count without applying those restrictions. The numeric count role is established here; a broader classification of the contained blocks is outside this recovery. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004C70B3–0x004C7186` and `0x004C69BB–0x004C6A82`; `decompiled/client/functions/004c69b0.c`.]

## Seed/material factor and ordered item products

For category 4 the category factor is binary32 one. Categories 3, 5, 6 and 7 use binary32 0.5.

The raw 32-bit seed is shifted left three bits within a 32-bit word; bits shifted out are discarded. The resulting word is divided as an unsigned integer by 21, and the remainder is retained. Thus a seed whose high bit is set is not treated as a negative dividend. The remainder lies between 0 and 20. It is converted to binary64 through the native integer-conversion sequence, which selects and adds the binary64 positive-zero table entry, and then is converted to binary32. The binary32 remainder is divided by binary32 twenty. The resulting binary32 quotient is subtracted from binary32 one, and binary32 one is added to that rounded difference. The subtraction and subsequent addition are distinct rounding boundaries; the factor is not formed by subtracting the quotient directly from two.

The material byte is then checked. Material 1 adds binary32 one to the completed seed factor. Material 26 adds binary32 0.5. Material 27 adds binary32 0.75. Every other material value leaves the factor unchanged. Only one such increment can occur. Each increment, when present, is one binary32 addition. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004C70D2–0x004C7154`.]

For effective item level, the signed 32-bit extra-block count is converted to binary32 and multiplied by the stored binary32 0.1 coefficient. Separately, the signed 16-bit item level is sign-extended and converted to binary32. The rounded count product is the first operand of a binary32 addition with that converted item level. This binary32 sum and the zero-extended unsigned item-rank byte are the inputs to RVA `0x045F60`. There is no integer cast of the effective level, no replacement of negative item levels or counts with zero, and no cap on the count in this calculation.

The returned continuous power multiplier is stored as binary32. The item-health result is then formed by three successive binary32 multiplications, in this order: the power multiplier by five; that rounded product by the category factor; and that rounded product by the completed seed/material factor. The final binary32 value is stored and returned through the x87 stack. There is no item-health clamp or random stat roll. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004C7159–0x004C71B3`.]

## Relationship to healing and level-up

The same query is used as a health ceiling by original healing paths. For example, in the update routine at RVA `0x20C510`, VAs `0x0060C5C8–0x0060C63F` obtain maximum health for a proportional restoration, obtain it again for a comparison, and obtain it again on the branch that writes the ceiling to current health at `+0x16C`. A role-5 restoration in RVA `0x03ACE0` also uses this query. Those callers' health writes are caller behavior; they are not mutations made by the query or its fixed item-health/power dependencies. This contract does not expand to all restoration sources, eligibility rules or timing.

The native level-up routine at RVA `0x047B00` updates the signed level field at `+0x190` before invoking this query and stores the result as binary32 current health at `+0x16C`. The calculation therefore uses the incremented level and the equipment/role/class/rank state present at each invocation. The caller can repeat the level-up body; the health query has no internal progression loop. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0060C5C8–0x0060C63F`, `0x0043ADFD–0x0043AE63` and `0x00447BA3–0x00447BB9`; `decompiled/client/functions/0043ace0.c` and `00447b00.c`.]

## Mutation, random draws and remaining numeric limits

The bounded native routines at RVAs `0x044DB0`, `0x0C70B0` and `0x045F60` make no writes to supplied creature or item records, consume no game RNG state, perform no seed mutation, and have no application-state side effects. Their seed-derived variation is deterministic unsigned arithmetic. Maximum health performs two imported power calls for a nonzero role, or three for role zero, before equipment. Each accepted equipment addition performs two more imported power calls. A category-rejected item-helper invocation performs none. No result is cached by these routines. [Evidence: the full instruction bodies at those RVAs in `decompiled/client/assembly.txt`.]

This contract closes the game-owned field reads, category/role/class decisions, constant bits, arithmetic ordering, integer signedness and conversion/storage boundaries for the scoped path. It leaves the imported `MSVCR110.DLL::_libm_sse2_pow_precise` implementation unresolved. The evidence establishes its binary64 operands and returned-result narrowing, not a platform-independent last-bit rule for exponentiation, special-value handling, runtime exception behavior or CRT-internal state. No runtime DLL implementation was inspected.

The scoped game routines do not establish a local floating-point control state or save and replace the caller's control settings. Their SSE operations and narrowing conversions therefore retain the native dependence on ambient MXCSR rounding and exception controls, including denormal handling. Their x87 loads/stores and returns also remain subject to the ambient x87 controls where those controls are applicable. A default round-to-nearest environment was not independently established for every call site in this recovery. Division by zero, nonfinite inputs/results, overflow, underflow, exception flags, traps and NaN payload propagation must consequently be understood within that ambient-control and imported-math limit; no additional game-owned sanitization is present in the bounded path.

The role-zero replacement, preliminary products, individual item products and additions, and all stated binary32/binary64 boundaries remain explicit even when real-number algebra could simplify them. This is an original-image behavioral contract, with no claim that an arbitrary host power function, reassociated evaluation, extended-precision accumulator or saturated input policy has identical numeric behavior. Native runtime acceptance and platform-wide math/control-state equivalence remain open beyond this bounded recovery.
