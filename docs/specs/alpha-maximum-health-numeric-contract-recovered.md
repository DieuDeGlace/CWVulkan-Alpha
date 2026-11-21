# Alpha maximum-health numeric contract — independently recovered

**FINAL / STABLE / HELD**

Status: re-certified 2026-11-21 against the original client and dedicated-server images. This is a bounded original-evidence contract for the maximum-health query and its numeric dependencies. It does not certify a complete health, combat, progression or replacement-engine system.

## Supported images and evidence

Two original images are supported. Both have preferred image base `0x00400000`. All evidence paths in this document are relative to the original evidence root, `disassembled alpha cube world`. Routine identifiers below are RVAs; an instruction VA is identified explicitly when used to locate evidence.

| Image | SHA-256 |
| --- | --- |
| Client `binary/Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| Dedicated server `binary/Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

The two images carry the same maximum-health behavior at different addresses. Every numeric claim in this document holds for both images; the instruction sequences, operand widths, field offsets, branch structure and constant bit patterns were compared one for one.

| Role | Client RVA | Server RVA |
| --- | --- | --- |
| Maximum-health query | `0x044DB0` | `0x00FDA0` |
| Fixed item-health helper | `0x0C70B0` | `0x013CE0` |
| Continuous level/rank power | `0x045F60` | `0x010F00` |
| Imported power thunk | `0x28D8D4` | `0x14AB14` |
| Import address slot used by the thunk | `0x2FC454` | `0x1582F8` |
| Class dispatch table (four 32-bit entries) | `0x044FE0` | `0x00FFD0` |

The primary evidence is the original images together with `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. The corresponding original decompilations are `decompiled/client/functions/00444db0.c`, `004c70b0.c` and `00445f60.c`, and `decompiled/server/functions/0040fda0.c`, `00413ce0.c` and `00410f00.c`. Their recovered signatures omit the register arguments to the imported math routine, and the power routine's recovered signature also omits its two stack parameters, so the original instructions establish the numeric contract. In both images the thunk is a single indirect jump through the listed import address slot, and the PE import directory resolves that slot to `MSVCR110.dll` entry `_libm_sse2_pow_precise`; `decompiled/client/external_functions.csv` and `decompiled/server/external_functions.csv` agree.

This recovery used original evidence only. No reconstruction, replacement-engine source, test, capture, probe output or implementation handoff was consulted.

## Query inputs, result and field roles

The maximum-health query receives one creature record and returns the maximum-health value as a binary32 value carried on the x87 return stack. The final return loads a stored binary32 value; there is no extra extended-precision health accumulation. The query does not read current health at creature offset `+0x16C` and does not write health or any creature/item field.

| Creature-relative field | Native read width and interpretation | Role in this query |
| --- | --- | --- |
| `+0x060` | One byte, compared for equality with zero | Controller/role discriminator; zero selects the fixed role-zero rank factor |
| `+0x140` | One byte, zero-extended and treated as an unsigned class identifier for dispatch | Class scaling, with specific cases 1, 2 and 4 |
| `+0x141` | One byte, equality comparison | An additional specialization factor only when both class and specialization are 1 |
| `+0x178` | Binary32 | Stored creature health coefficient, multiplied directly into the creature contribution |
| `+0x190` | Signed 32-bit integer converted to binary32 | Creature level |
| `+0x1A8` | Unsigned byte, zero-extended before conversion to binary32 | Creature rank used by the initial rank power call |
| Equipment records in the table below | Each begins with a one-byte category | Potential fixed item-health additions |

The byte at `+0x060`, the class byte and the specialization byte are distinct inputs. This contract calls `+0x060` the controller/role byte; the evidence here establishes only its numeric test, not a name for each value. The branch condition is exact: when the byte equals zero, the role-zero replacement described below is taken; when it is any nonzero value, it is skipped. No distinction among nonzero values is made in this query. The controller/role decision occurs before class and specialization scaling. Class scaling and the class-1/specialization-1 factor remain active for role zero. [Evidence: client VAs `0x00444DC4–0x00444ED5`, decision at `0x00444E2E` and `0x00444E4D`; server VAs `0x0040FDB4–0x0040FEC5`, decision at `0x0040FE1E` and `0x0040FE3D`.]

The stored health coefficient is not clamped, replaced with a universal base-health constant, or multiplied by the class factor after equipment has been added. Its existing binary32 value is the input. This scope does not specify every writer that establishes that coefficient during creature creation or subsequent state changes.

## Exact constants

The following are the stored constants used by the bounded path. Decimal labels identify their intended magnitudes; the bit patterns specify the values actually supplied by both images. In particular, the labels 0.05, 0.1, 1.1, 1.2 and 1.3 denote the listed binary32 values, not exact decimal real numbers.

| Use / decimal label | Format | Bit pattern | Client RVA | Server RVA |
| --- | --- | --- | --- | --- |
| Arithmetic one | Binary32 | `0x3F800000` | `0x345DC0` | `0x1586DC` |
| Level coefficient, 0.05 | Binary32 | `0x3D4CCCCD` | `0x2FCDB4` | `0x1586B8` |
| Level exponent scale, 3 | Binary32 | `0x40400000` | `0x2FCD70` | `0x158718` |
| Power base, 2 | Binary64 | `0x4000000000000000` | `0x345E18` | `0x1588D8` |
| Rank exponent coefficient, 0.25 | Binary32 | `0x3E800000` | `0x345D98` | `0x1586C8` |
| Role-zero exponent, 1 | Binary64 | `0x3FF0000000000000` | `0x345DF8` | `0x1588D0` |
| Class 1 factor, 1.3 | Binary32 | `0x3FA66666` | `0x2FD8E4` | `0x1588CC` |
| Class 2 factor, 1.1 | Binary32 | `0x3F8CCCCD` | `0x2FD780` | `0x1588C4` |
| Class 4 factor, 1.2 | Binary32 | `0x3F99999A` | `0x2FCDCC` | `0x1586E0` |
| Class 1 / specialization 1 factor, 1.25 | Binary32 | `0x3FA00000` | `0x2FCDD0` | `0x1586E4` |
| Non-chest item category factor / material-26 increment, 0.5 | Binary32 | `0x3F000000` | `0x345DA0` | `0x1586D0` |
| Seed-remainder divisor, 20 | Binary32 | `0x41A00000` | `0x2FCD74` | `0x15874C` |
| Material-27 increment, 0.75 | Binary32 | `0x3F400000` | `0x2FCDC4` | `0x1586D4` |
| Extra-block count coefficient, 0.1 | Binary32 | `0x3DCCCCCD` | `0x2FCB7C` | `0x1586BC` |
| Fixed item-health scale, 5 | Binary32 | `0x40A00000` | `0x2FCE24` | `0x15872C` |
| Integer-conversion table entry 0, the one selected for the remainder | Binary64 | `0x0000000000000000` | `0x345F30` | `0x15AC20` |
| Integer-conversion table entry 1, never selected here | Binary64 | `0x41F0000000000000` | `0x345F38` | `0x15AC28` |

These values were read directly from the two images after resolving their PE section mappings; every listed constant lies in the read-only data section. The original instructions at the scoped routines identify the operand widths and constant addresses.

The class dispatch is a bounded table jump. The class byte is zero-extended, one is subtracted, and the result is compared as an unsigned 32-bit value against 3; any result above 3 (that is, class 0 or any class above 4) bypasses the table and receives no class factor. Within range, the four table entries select, in order: the class 1 factor, the class 2 factor, a target that skips the class multiplication entirely (class 3), and the class 4 factor. The client table holds `0x00444E95`, `0x00444EA9`, `0x00444EBB` and `0x00444E9F`; the server table holds `0x0040FE85`, `0x0040FE99`, `0x0040FEAB` and `0x0040FE8F`. [Evidence: constant RVAs in the table above; client VAs `0x00444E7F–0x00444ED5`; server VAs `0x0040FE6F–0x0040FEC5`.]

## Continuous level/rank power

The continuous power routine consumes a binary32 level as its first stack argument and a signed 32-bit rank as its second stack argument; the caller removes both arguments. In the fixed item-health path, the caller has already widened the item rank from an unsigned byte, so that rank lies between 0 and 255. There is no rank clamp in this power routine.

The level exponent is constructed in binary32. Starting from the supplied level, the native arithmetic subtracts one, multiplies that rounded difference by the stored 0.05 coefficient, and adds one. A binary32 one is divided by that rounded sum. The quotient is subtracted from a binary32 one, and the resulting difference is multiplied by binary32 three. Every subtraction, multiplication, addition and division in this paragraph produces a binary32 result before the next operation. There is no integer conversion or truncation of this exponent.

That completed binary32 exponent is widened to binary64 and supplied as the exponent of the first imported power call, with binary64 two as the base. The imported binary64 result is converted to binary32 immediately and saved. The rank integer is separately converted to binary32 as a signed integer, multiplied by binary32 0.25, and widened to binary64 for the second imported power call, again with binary64 two as the base. Its binary64 result is immediately converted to binary32. The saved level factor is then multiplied by the rank factor in binary32, with the level factor as the first operand. That stored binary32 product is loaded onto the x87 return stack. [Evidence: client VAs `0x00445F60–0x00445FE5`; server VAs `0x00410F00–0x00410F85`.]

The maximum-health query uses this same level-exponent arithmetic directly for the creature's signed 32-bit level, including an initial signed-integer-to-binary32 conversion. Its rank input is the unsigned byte at `+0x1A8`. The creature path calculates its own powers directly rather than calling the continuous power routine. Neither health path obtains its continuous multiplier from the separate integer-valued level query (client RVA `0x045F10`), which uses a different scale constant (binary32 `0x42C80000`, client RVA `0x2FD0B4`) and truncates its result to an integer. There is no experience read, level floor, level ceiling, adaptation to a receiving creature's level, or integer power quantization in the bounded health path. [Evidence: client VAs `0x00444DC4–0x00444E29`, `0x00445F10–0x00445FE5` and `0x004C7159–0x004C718B`; server VAs `0x0040FDB4–0x0040FE19` and `0x00413D89–0x00413DBB`.]

## Creature contribution and role/class precedence

The first imported power call calculates the creature level factor using the level exponent just described. Its result is narrowed to binary32 and retained. The second imported power call uses the zero-extended creature rank, converted to binary32 and multiplied by binary32 0.25, widened to binary64, as its exponent. Its narrowed binary32 result is multiplied first by the retained level factor and then by the stored health coefficient at `+0x178`, with a binary32 result at each multiplication.

The controller/role comparison follows those two imported power calls. For any nonzero byte, their completed product is retained. When the byte is zero, a third imported power call supplies binary64 two raised to the fixed binary64 exponent one. That result is narrowed to binary32, multiplied first by the retained level factor and then by the same stored health coefficient, again with a binary32 boundary at each product. This replaces the preliminary rank-dependent creature contribution. Consequently, role zero still evaluates the creature-rank power and preliminary products before the role-zero replacement; their imported-call and floating-point-status effects are part of the observed ordering even though their numeric product is superseded. [Evidence: client VAs `0x00444DFA–0x00444E7A`; server VAs `0x0040FDEA–0x0040FE6A`.]

The selected creature contribution is then multiplied in binary32 by the class factor: class 1 uses 1.3, class 2 uses 1.1, and class 4 uses 1.2, with the exact bits listed above. Every other class byte receives no multiplication at this stage. After that class operation, class 1 with specialization 1 receives a further binary32 multiplication by 1.25. There is no corresponding specialization multiplication for any other class or specialization value. Equipment health is added only after these operations, so these class factors apply to the creature contribution rather than to the equipment additions. [Evidence: client VAs `0x00444E7F–0x00444EDA`; server VAs `0x0040FE6F–0x0040FECA`; dispatch tables as listed above.]

## Equipment eligibility and accumulation order

The native query examines exactly these six fixed records, in the following order. Each accepted record is passed to the fixed item-health helper; its returned binary32 item-health value is added to the current binary32 total before the next record is considered.

| Addition order | Creature-relative record start | Required category byte at the record start |
| --- | --- | --- |
| 1 | `+0x990` | 3 |
| 2 | `+0xAA8` | 3 |
| 3 | `+0x530` | 4, native category name `ChestArmor` |
| 4 | `+0x648` | 6, native category name `Boots` |
| 5 | `+0x760` | 5, native category name `Gloves` |
| 6 | `+0x878` | 7, native category name `ShoulderArmor` |

A record whose category does not match its particular gate contributes nothing and the helper is not called for it, even if that category would be accepted by the item helper on its own. No other equipment record contributes in this query. There is no subtype, class-compatibility, material, item-level, identification, count-of-equipped-items or receiving-creature-level gate before these six calls. Both category-3 records may independently contribute. Every gate category is one the helper accepts, so the helper's category-rejection path is never reached from this query. [Evidence: client VAs `0x00444EDA–0x00444FD8`; server VAs `0x0040FECA–0x0040FFC8`. Category names are established by original client dictionary assignments at VAs `0x005901BA–0x00590251` (within client routine RVA `0x18EB00`), which pair keys 6, 5, 4 and 7 with the UTF-16 strings at client RVAs `0x31B320`, `0x31B32C`, `0x31B33C` and `0x31B354`, with the matching entries in `decompiled/client/strings.csv`.]

At each accepted addition, the helper's x87 result is stored as binary32, then loaded as the first scalar addition operand; the previous total is the second operand. The binary32 sum becomes the total for the following slot. The final returned total is a stored binary32 value. The query performs no terminal minimum, maximum, nonnegative clamp or saturation. [Evidence: client VAs `0x00444EE9–0x00444FDE`; server VAs `0x0040FED9–0x0040FFCE`.]

## Fixed item-health inputs and category rejection

The fixed item-health helper accepts one item record. An item category other than 3, 4, 5, 6 or 7 returns positive zero immediately, through the x87 zero constant. On that rejected path it does not read the seed, rank, material, item level or extra-block count and does not call the power dependency. As stated above, this path is reachable only from other callers of the helper.

For an accepted category, the following fields are consumed. Other item fields, including subtype at `+0x01`, are not consulted by this helper.

| Item-relative field | Native interpretation | Numeric role |
| --- | --- | --- |
| `+0x00` | One-byte category | Eligibility and the category factor |
| `+0x04` | Raw 32-bit word | Deterministic seed variation after a wrapping left shift |
| `+0x0C` | Unsigned byte, zero-extended to 32 bits | Item rank/rarity argument for continuous power |
| `+0x0D` | Unsigned byte | Material identifier selecting a seed-factor increment |
| `+0x10` | Signed 16-bit integer, sign-extended | Raw item level component |
| `+0x114` | Signed 32-bit integer | Extra-block count contributing a fractional level component |

The count role at `+0x114` is also supported by native client item-record insertion at RVA `0x0C69B0`, which uses it as the number of eight-byte records beginning at item offset `+0x14`. That writer's ordinary size restrictions are separate from this health helper: the helper reads and uses the entire signed count without applying those restrictions. The numeric count role is established here; a broader classification of the contained blocks is outside this recovery. [Evidence: client VAs `0x004C70B3–0x004C7186` and `0x004C69BB–0x004C6A82`; server VAs `0x00413CE3–0x00413DB6`; `decompiled/client/functions/004c69b0.c`.]

## Seed/material factor and ordered item products

For category 4 the category factor is binary32 one. Categories 3, 5, 6 and 7 use binary32 0.5.

The raw 32-bit seed is shifted left three bits within a 32-bit word; bits shifted out are discarded. The resulting word is divided as an unsigned integer by 21, and the remainder is retained. Thus a seed whose high bit is set is not treated as a negative dividend. The remainder lies between 0 and 20. It is converted to binary64 as a signed integer, then the native integer-conversion sequence adds the table entry indexed by the remainder's bit 31; because the remainder is at most 20, entry 0 (binary64 positive zero) is always selected. The result is converted to binary32. The binary32 remainder is divided by binary32 twenty. The resulting binary32 quotient is subtracted from binary32 one, and binary32 one is added to that rounded difference. The subtraction and subsequent addition are distinct rounding boundaries; the factor is not formed by subtracting the quotient directly from two.

The material byte is then checked. Material 1 adds binary32 one to the completed seed factor. Material 26 adds binary32 0.5. Material 27 adds binary32 0.75. Every other material value leaves the factor unchanged. Only one such increment can occur. Each increment, when present, is one binary32 addition with the seed factor as the first operand. [Evidence: client VAs `0x004C70D2–0x004C7154`; server VAs `0x00413D02–0x00413D84`.]

For effective item level, the signed 32-bit extra-block count is converted to binary32 and multiplied by the stored binary32 0.1 coefficient. Separately, the signed 16-bit item level is sign-extended and converted to binary32. The rounded count product is the first operand of a binary32 addition with that converted item level. This binary32 sum and the zero-extended unsigned item-rank byte are the inputs to the continuous power routine. There is no integer cast of the effective level, no replacement of negative item levels or counts with zero, and no cap on the count in this calculation.

The returned continuous power multiplier is stored as binary32. The item-health result is then formed by three successive binary32 multiplications, in this order: the power multiplier by five; that rounded product by the category factor; and that rounded product by the completed seed/material factor. The final binary32 value is stored and returned through the x87 stack. There is no item-health clamp or random stat roll. [Evidence: client VAs `0x004C7159–0x004C71B3`; server VAs `0x00413D89–0x00413DE3`.]

## Relationship to healing and level-up

The same query is used as a health ceiling by original healing paths. For example, in the client update routine at RVA `0x20C510`, VAs `0x0060C5C8–0x0060C63F` obtain maximum health for a proportional restoration, obtain it again for a comparison, and obtain it again on the branch that writes the ceiling to current health at `+0x16C`; the server routine at RVA `0x1322D0` does the same at VAs `0x00532388–0x005323FF`. A restoration taken when the controller/role byte equals 5, in client RVA `0x03ACE0` (VAs `0x0043ADF5–0x0043AE5D`) and server RVA `0x005780` (VAs `0x00405895–0x004058FD`), also uses this query three times in the same pattern. Those callers' health writes are caller behavior; they are not mutations made by the query or its fixed item-health/power dependencies. This contract does not expand to all restoration sources, eligibility rules or timing.

The native level-up routine (client RVA `0x047B00`, server RVA `0x012550`) does nothing unless the signed level at `+0x190` is greater than zero. Each iteration subtracts the current level's experience requirement from the signed experience field at `+0x194`, stores the incremented level at `+0x190`, then invokes this query and stores its result as binary32 current health at `+0x16C`. The routine itself repeats that body while the remaining experience is at least the new level's requirement, so a single level-up call may invoke the query several times, each with the incremented level and the equipment/role/class/rank state then present. The experience-requirement formula belongs to the level-up contract, not to this query; the health query itself has no internal progression loop. [Evidence: client VAs `0x00447B03–0x00447C16`, query call at `0x00447BB4`; server VAs `0x00412553–0x00412666`, query call at `0x00412604`; `decompiled/client/functions/0043ace0.c` and `00447b00.c`.]

## Mutation, random draws and remaining numeric limits

The bounded native routines (the maximum-health query, the fixed item-health helper and the continuous power routine, in both images) make no writes to supplied creature or item records, consume no game RNG state, perform no seed mutation, and have no application-state side effects. Their seed-derived variation is deterministic unsigned arithmetic. Maximum health performs two imported power calls for a nonzero controller/role byte, or three when it is zero, before equipment. Each accepted equipment addition performs two more imported power calls. A category-rejected item-helper invocation performs none. No result is cached by these routines. [Evidence: the full instruction bodies at the listed RVAs in `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`; call lists in `decompiled/client/calls.csv` and `decompiled/server/calls.csv`.]

This contract closes the game-owned field reads, category/role/class decisions, constant bits, arithmetic ordering, integer signedness and conversion/storage boundaries for the scoped path. It leaves the imported `MSVCR110.DLL::_libm_sse2_pow_precise` implementation unresolved. The evidence establishes its binary64 operands (base in the first SSE register, exponent in the second) and returned-result narrowing, not a platform-independent last-bit rule for exponentiation, special-value handling, runtime exception behavior or CRT-internal state. No runtime DLL implementation was inspected.

The scoped game routines do not establish a local floating-point control state or save and replace the caller's control settings. Their SSE operations and narrowing conversions therefore retain the native dependence on ambient MXCSR rounding and exception controls, including denormal handling. Their x87 loads/stores and returns also remain subject to the ambient x87 controls where those controls are applicable. A default round-to-nearest environment was not independently established for every call site in this recovery. Division by zero, nonfinite inputs/results, overflow, underflow, exception flags, traps and NaN payload propagation must consequently be understood within that ambient-control and imported-math limit; no additional game-owned sanitization is present in the bounded path.

The role-zero replacement, preliminary products, individual item products and additions, and all stated binary32/binary64 boundaries remain explicit even when real-number algebra could simplify them. This is an original-image behavioral contract, with no claim that an arbitrary host power function, reassociated evaluation, extended-precision accumulator or saturated input policy has identical numeric behavior. Native runtime acceptance and platform-wide math/control-state equivalence remain open beyond this bounded recovery.

## Re-certification (2026-11-21)

Every claim was re-checked against the original instructions in `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt` and against bytes read directly from `binary/Cube.exe` and `binary/Server.exe` (both SHA-256 values re-computed). No reconstruction was consulted; no other held specification is cited by this contract. The earlier numeric description of the client path was confirmed without numeric error. Corrections and additions:

1. Status line: the held marker now appears directly under the title; the earlier status sentence relied on an unstated readback. [Document header.]
2. Scope omission: the dedicated server image `binary/Server.exe` was not covered. Its maximum-health query (RVA `0x00FDA0`), item-health helper (RVA `0x013CE0`), power routine (RVA `0x010F00`), power thunk (RVA `0x14AB14`) and dispatch table (RVA `0x00FFD0`) were verified to be behaviorally identical, and server constant RVAs were added. [Server VAs `0x0040FDA0–0x0040FFCE`, `0x00413CE0–0x00413DE3`, `0x00410F00–0x00410F85`.]
3. Level-up repetition was misattributed: the earlier text said the caller can repeat the level-up body. The repetition is internal to the level-up routine itself, which loops back while the experience at `+0x194` meets the next requirement, and it is gated by a signed level greater than zero. [Client VAs `0x00447B09`, `0x00447C10–0x00447C16`; server VAs `0x00412559`, `0x00412660–0x00412666`.]
4. Import identity was supported only by an exporter listing; it is now confirmed from each image's PE import directory through the thunk's import address slot (client RVA `0x2FC454`, server RVA `0x1582F8`), and the operand registers of the imported call are stated. [Client VA `0x0068D8D4`; server VA `0x0054AB14`.]
5. Decompilation caveat refined: the power routine's recovered signature also omits its two stack parameters, not only the imported routine's register arguments. [`decompiled/client/functions/00445f60.c`.]
6. Omission: every gate category used by the query is accepted by the helper, so the helper's category-rejection path is unreachable from the maximum-health query and the helper is not called for a non-matching record. [Client VAs `0x00444EDA–0x00444FBF` against `0x004C70B3–0x004C70CA`.]
7. Class dispatch detail added: the unsigned range test that sends class 0 and classes above 4 past the table, the actual table entries, and the class-3 entry that skips the multiplication. [Client VAs `0x00444E85–0x00444E8E`, table RVA `0x044FE0`; server VAs `0x0040FE75–0x0040FE7E`, table RVA `0x00FFD0`.]
8. Integer-conversion table detail completed: the earlier text named only the selected entry; the table has two entries indexed by the remainder's bit 31, and the never-selected entry 1 (`0x41F0000000000000`) was added. [Client VA `0x004C7110–0x004C7113`, RVA `0x345F38`; server RVA `0x15AC28`.]
9. Controller/role byte at `+0x060`: the zero branch condition is now stated explicitly (equal to zero takes the replacement), and the byte is named as the controller/role discriminator without assigning value names. [Client VAs `0x00444E2E`, `0x00444E4D`.]
10. Category-name evidence made specific: the dictionary assignments pair keys 6, 5, 4 and 7 with UTF-16 strings at client RVAs `0x31B320`, `0x31B32C`, `0x31B33C` and `0x31B354`, within client routine RVA `0x18EB00`. [Client VAs `0x005901BA–0x00590251`.]
11. Caller evidence extended to the server healing routine (RVA `0x1322D0`) and the server value-5 restoration (RVA `0x005780`); the client value-5 restoration VA range was corrected to begin at the value-5 test. [Client VA `0x0043ADF5`; server VAs `0x00532388–0x005323FF`, `0x00405895–0x004058FD`.]
12. Operand order made explicit for the power routine's final product (level factor first) and for the material increment (seed factor first); the separate integer-valued level query's distinct scale constant and truncation were identified. [Client VAs `0x00445FD8`, `0x004C7140–0x004C7150`, `0x00445F41–0x00445F4D`; RVA `0x2FD0B4`.]

Remaining uncertainties:

- The imported `_libm_sse2_pow_precise` last-bit results, special-value handling and CRT-internal state remain unresolved; no runtime DLL was inspected.
- The ambient MXCSR and x87 control state at every call site was not established.
- The semantic names of the controller/role byte values, the class values and the specialization values are not established by this evidence beyond the numeric tests stated.
- All writers of the stored health coefficient at `+0x178` remain outside this scope.
- The server's item-record insertion writer for the `+0x114` count was not separately located; the count role rests on the client writer plus the identical server helper reads.
- Native runtime acceptance of this contract has not been performed by this author.

Cross-spec conflicts: none recorded; this contract cites no other held specification, and no other specification was read.
