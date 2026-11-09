# Alpha common-action hit-input construction

Status: FINAL / STABLE / HELD after complete prose readback by the author.

## Image, scope and evidence

This contract covers the inputs constructed before the call to generic hit resolution at RVA `0x21476B`, with the principal construction beginning at RVA `0x2144A8`. The enclosing original world-update routine starts at RVA `0x20C510`. The supported image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. All evidence paths are relative to the original `disassembled alpha cube world` root. Addresses are RVAs unless explicitly marked VAs.

Primary evidence is `decompiled/client/assembly.txt`, VAs `0x00614441–0x0061476B`, with the necessary earlier charge, flag and baseline preparation identified below; original helper bodies in that file; the corresponding files under `decompiled/client/functions/`; and original constants and dispatch tables in `binary/Cube.exe`. The original assembly export omits some branch bodies and cleanup tails. Where needed, the original binary supplies those bytes directly; no replacement implementation or reconstruction fills those gaps.

This contract begins with an attacker and candidate target that have reached this construction path. It does not close target discovery, attack timing, collision/line-of-sight eligibility, projectile behavior, or the downstream generic hit resolver. The resolver's body was not audited for this task. Existing held contracts and other specifications were left unopened and unchanged.

## Fields and action identities used here

| Attacker-relative field | Native input role |
| --- | --- |
| `+0x008` | Eight-byte creature identifier, used in the target-hit bookkeeping |
| `+0x010` | Three consecutive signed 64-bit position components |
| `+0x060` | One-byte numeric role; roles 3 and 5 force the vector-strength helper to return zero |
| `+0x068` | One-byte action, zero-extended where passed as an action argument |
| `+0x070` | Signed 32-bit combo counter used by the speed dependency |
| `+0x07E` | 16-bit creature flags, including fallback-damage bits 8 and 16 |
| `+0x080` | First binary32 size component; the target's corresponding component is the divisor for vector scaling |
| `+0x140` | One-byte class identifier used by the speed dependency |
| `+0x141` | One-byte specialization, used by the speed dependency |
| `+0x17C` | Binary32 coefficient used to obtain the integer combo cap |
| `+0x180` | Binary32 stored creature-damage coefficient |
| `+0x190` / `+0x1A8` | Signed 32-bit level / unsigned-byte rank, consumed by scalar dependencies |
| `+0x990` and `+0xAA8` | Equipment records, with category at record byte zero and subtype at record byte one |
| `+0x1178` | Status-list pointer used by critical and speed dependencies |
| `+0x1190` | Binary32 additional critical-threshold input |
| `+0x11AC` | Target-hit collection for the current attack cycle |
| `+0x13B4` | Binary32 stored resource/charge amount used in damage and forwarded separately |
| `+0x13B8` | One-byte hit-attempt marker, set before entering generic hit resolution |
| `+0x13C0` | One-byte latched critical input |

The original UI dispatch names actions 30, 31, 32 and 33 as **Fire Swirl**, **Fire Vortex**, **Water Swirl** and **Water Vortex**. It names 87 **Explosion** and 88 **Fire Explosion**. These names are established by the action-byte dispatch at VA `0x0049320C`, its branch-index table at RVA `0x09D430`, target table at RVA `0x09D3E0`, and original binary branches at VAs `0x00493297–0x00493433` and `0x00493839`, together with the corresponding entries in `decompiled/client/strings.csv`. Names for other numeric action branches are not inferred here.

Actions 30–33, 87 and 88 all select the magic-stat damage branch in this caller. The separate magic flag supplied to generic hit resolution is true only for actions 30–33. Thus the damage-stat branch and the forwarded magic flag have different memberships. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0061452E–0x00614618` and `0x006146F0–0x00614714`.]

## Numeric conventions and constants

All scalar arithmetic described as binary32 has a native binary32 boundary after each stated addition, subtraction, multiplication or division. The caller stores x87 scalar-helper returns to binary32 before using them in SSE arithmetic. The order of those operations remains significant. No fused expression, integer damage rounding, damage floor or damage saturation is performed by this caller before the generic-hit call.

| Constant label | Binary32 bits | Original constant RVA |
| --- | --- | --- |
| Positive zero | `0x00000000` | `0x345D84` |
| One | `0x3F800000` | `0x345DC0` |
| 0.01, horizontal squared-length threshold | `0x3C23D70A` | `0x2FCDA0` |
| One divided by 65536 | `0x37800000` | `0x2FCD94` |
| 0.05, level-curve coefficient | `0x3D4CCCCD` | `0x2FCDB4` |
| 0.1, skill/count coefficient | `0x3DCCCCCD` | `0x2FCB7C` |
| 0.15, extra critical-threshold coefficient | `0x3E19999A` | `0x2FCDB8` |
| 0.25 | `0x3E800000` | `0x345D98` |
| 0.4, actions 30 and 32 | `0x3ECCCCCD` | `0x2FD318` |
| 0.5 | `0x3F000000` | `0x345DA0` |
| 0.6, actions 31 and 33 | `0x3F19999A` | `0x2FD8DC` |
| 0.75 | `0x3F400000` | `0x2FCDC4` |
| 0.0625, nonzero-role speed/rank coefficient | `0x3D800000` | `0x2FD8D0` |
| 1.25, random damage-factor starting value | `0x3FA00000` | `0x2FCDD0` |
| 2 | `0x40000000` | `0x345E10` |
| 3 | `0x40400000` | `0x2FCD70` |
| 4 | `0x40800000` | `0x345E60` |
| 5 | `0x40A00000` | `0x2FCE24` |
| 6 | `0x40C00000` | `0x345E64` |
| 8 | `0x41000000` | `0x345E70` |
| 10 | `0x41200000` | `0x345E74` |
| 20 | `0x41A00000` | `0x2FCD74` |
| 30 | `0x41F00000` | `0x2FCD78` |
| 50 | `0x42480000` | `0x2FCD7C` |
| 60 | `0x42700000` | `0x2FCE48` |
| 80 | `0x42A00000` | `0x2FCE50` |
| 32767, random-draw divisor | `0x46FFFE00` | `0x2FD338` |

Decimal labels such as 0.1, 0.15, 0.4 and 0.6 mean the listed binary32 values. Binary64 two for imported powers is stored at RVA `0x345E18`, bits `0x4000000000000000`. The vertical vector component is supplied by the immediate binary32 bits `0x3E800000`. These constants and the tables cited below were read from the supported original image.

## Critical input and random-draw order

At the beginning of the principal block, a nonzero attacker byte at `+0x13C0` supplies critical true directly. The ordinary critical helper at RVA `0x0444A0` is skipped, so this latched path consumes no critical-decision draw. A zero latch invokes that helper and uses its Boolean return. The latch is neither cleared nor consumed by this block. [Evidence: `decompiled/client/assembly.txt`, VAs `0x006144B4–0x006144D6`.]

For valid status-list records, the ordinary helper searches for status identifier 11, stored as a byte at each node's `+0x08`. A matching status supplies a binary32 threshold of one and bypasses the underlying critical-stat query. Without that status, the helper obtains the scalar critical-stat result from RVA `0x03EA40`, stores it as binary32, and adds a binary32 product of attacker `+0x1190` and 0.15. That product is formed first; the stat result is the first operand of the following addition. No threshold clamp is applied.

Both ordinary-helper paths then call the imported `rand` exactly once. The returned EAX value is interpreted as a signed 32-bit integer and converted to binary32, then divided by binary32 32767. The helper returns true only when the threshold is ordered and strictly greater than that normalized draw. Equality and unordered comparison return false. A status-11 threshold of one therefore still consumes a draw and fails if the returned integer is exactly 32767; this differs from the draw-free latched-true path. [Evidence: `decompiled/client/assembly.txt`, VAs `0x004444A0–0x0044451E`; original `decompiled/client/functions/004444a0.c`.]

After the critical decision, vector-strength and vector-scaling calculations occur, then the selected damage branch obtains its action coefficient and damage stat. Each damage branch subsequently calls `rand` once for damage variation. The ordinary path therefore consumes the critical draw first and variation draw second. The latched path consumes only the variation draw in this bounded construction. The same stored critical decision is forwarded to generic hit resolution and, when true, causes one binary32 multiplication of the constructed damage by two before that call.

The variation draw also uses a signed 32-bit EAX-to-binary32 conversion. The converted draw is multiplied by binary32 0.5, then divided by binary32 32767; the quotient is subtracted from binary32 1.25. The physical branch adds binary32 positive zero to that rounded difference, then multiplies that factor by the prepared damage. The magic-stat branch multiplies the difference by its prepared damage, then multiplies the result by a separately formed binary32 positive-zero-plus-one value. These operations are retained as separate native boundaries. There is no modulo operation, integer division, random reseed, or independent per-weapon random roll in this construction. [Evidence: `decompiled/client/assembly.txt`, VAs `0x006145C7–0x00614694`. The call through VA `0x006FC468` resolves to `rand` in the original PE import data; `decompiled/client/external_functions.csv` identifies the imported function.]

The game-owned arithmetic, item-power and stat dependencies inspected for this path contain no additional game `rand` calls. This draw count covers construction up to the generic-hit call, not earlier world-update work, allocation/runtime internals, or draws performed by generic hit resolution and later effects.

## Action coefficients, charge and damage branch

RVA `0x03EC90` supplies the following binary32 action coefficient:

| Action identifiers | Coefficient |
| --- | --- |
| 5, 26 | 0.5 |
| 21, 72 | 0.1 |
| 30, 32 | 0.4 |
| 31, 33 | 0.6 |
| 37, 43, 57, 58, 68 | 2 |
| 55 | 0.25 |
| Every other action | 1 |

The original binary's dispatch index at RVA `0x03ED10` and target table at `0x03ECF0` confirm the membership, including the quarter coefficient at VA `0x0043ECB6` omitted from the assembly export's routine membership. [Evidence: `binary/Cube.exe`; `decompiled/client/assembly.txt`, VAs `0x0043EC90–0x0043ECEB`; `decompiled/client/functions/0043ec90.c`.]

In the physical branch, the current binary32 charge at attacker `+0x13B4` is saved before obtaining the action coefficient and damage stat from RVA `0x03DC60`. The action coefficient is multiplied first by the returned binary32 damage stat. Separately, the saved charge is squared in binary32, multiplied by five, and added to a baseline. Before candidate iteration, the baseline is established as positive zero for actions 11, 60 and 62 and one for every other action; the principal block reuses that saved baseline rather than recomputing it for each target. The coefficient/stat product is then multiplied by that completed charge factor. Only after this product does the damage-variation draw occur. The charge is not clamped or converted to an integer here. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00614084–0x006140AE` and `0x00614561–0x006145C7`.]

For actions 30–33, 87 and 88, the caller instead obtains the action coefficient and the damage stat from RVA `0x046E80`, multiplies the two binary32 values, then applies the variation sequence described above. It does not apply the physical branch's charge-square factor or baseline. After either branch, the stored critical decision conditionally doubles the damage.

The charge is also read afresh at attacker `+0x13B4` and forwarded as a separate binary32 argument immediately before the generic-hit call. The earlier charge-related Boolean forwarded by the caller is true when the stored charge was ordered and greater than positive zero at its preparation point, or when action 11 was selected; otherwise it is false. The relevant earlier path tests the charge after resource/charge preparation and stores the Boolean before candidate processing. Resource amounts, pending charge and resource-cost/timing helpers upstream of that preparation are not closed here. In particular, this contract does not infer a resource payment from the mere presence of a nonzero forwarded charge. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00613625–0x00613646`, `0x006137E9–0x006137F1`, and `0x0061473A–0x00614749`.]

## Damage base and selected equipment

The physical and magic-stat queries at RVAs `0x03DC60` and `0x046E80` begin with a creature contribution using signed level at `+0x190`, the stored binary32 damage coefficient at `+0x180`, and an initially fixed zero rank exponent. They then obtain selected equipment from RVA `0x03DF60`.

For the level factor, the native operations convert signed level to binary32; subtract one; multiply by 0.05; add one; divide one by that sum; subtract the quotient from one; and multiply the difference by three. The completed binary32 exponent is widened for an imported binary64 power of two, whose result is narrowed to binary32. A separate imported power of two receives the widened binary32 result of 0.25 multiplied by positive zero. That narrowed rank factor is multiplied by the saved level factor; the stored damage coefficient is then multiplied by that rounded product. These queries perform both imported power calls rather than replacing the zero-exponent call with a literal one.

Equipment selection uses category 3 as its weapon gate. First priority is the record at `+0xAA8` when its subtype is 6 or 7. If that priority fails, a category-3 record at `+0x990` with subtype 6 or 7 has second priority. A priority match selects that single record and bypasses the action-based choices below.

Without either priority match, the selection is:

| Action identifiers | Selected category-3 records |
| --- | --- |
| 2, 4, 7, 8, 10, 18, 39, 41 | `+0x990` only, if category 3 |
| 40, 42 | `+0xAA8` only, if category 3 |
| 11, 12, 16, 17, 20, 54, 86, 91, 96 | `+0x990` first, then `+0xAA8`, each independently gated by category 3 |
| Every other action | `+0xAA8` first if category 3, then `+0x990` only when it is category 3 and subtype 12 |

Each selected item's scalar damage from RVA `0x0C7F60` is stored as binary32 and added to the current binary32 creature contribution in selection order. The item contribution is the first addition operand and the accumulated total the second. No offhand percentage is applied by these queries.

When no weapon was selected, flag bit 8 at creature `+0x7E` adds a fresh level/rank power contribution multiplied by two. Flag bit 16 then adds another fresh level/rank power contribution multiplied by eight. These fallback contributions use the actual zero-extended rank byte at `+0x1A8`; they do not replace the initial creature contribution. Both bits may contribute, in that order. Each contribution uses narrowed binary32 power results and binary32 products/addition. With selected equipment present, neither fallback bit contributes through this branch.

The physical query has an additional final action multiplication: action 87 by 50, 91 by two, 93 by five, or 96 by three. The magic-stat query has no corresponding final action multiplication. In the principal common-hit caller, action 87 selects the magic-stat query, so the physical query's action-87 factor is not used for that call. The queries return their stored binary32 totals through the x87 return stack. Original binary cleanup tails at VAs `0x0043DF27` and `0x004470F7` establish those returns despite their omission from the exported instruction bodies. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0043DC8A–0x0043DF22`, `0x00446EAA–0x004470F2` and `0x0043DFA9–0x0043E0FF`; original binary dispatch tables at RVAs `0x03E120`, `0x03E130`, `0x03DF40` and `0x03DF54`; `decompiled/client/functions/0043df60.c`, `0043dc60.c` and `00446e80.c`.]

The complete numeric formula inside item-damage RVA `0x0C7F60`, the normalized critical-stat query RVA `0x03EA40` and its item dependency RVA `0x0C6BA0`, and the item/haste query RVA `0x0478B0` and its dependency RVA `0x0C7C00` are bounded scalar dependencies here. Their outputs are consumed at the binary32 boundaries identified in this contract. Inspection establishes arithmetic/power calls and no game RNG draws in those paths; it does not constitute a complete item-stat parity contract or authorize assumed substitutes for their numeric outputs.

## Hit-vector construction and scalar dependencies

The displacement is formed from target position minus attacker position, component by component, using three 64-bit subtractions. Native low-word subtraction and high-word borrow retain the resulting 64-bit bit patterns. RVA `0x02C4A0` interprets each as signed 64-bit, converts through the x87 stack to stored binary32, then multiplies by binary32 one divided by 65536. This conversion precedes the scale multiplication; there is no binary64 world-distance accumulator.

The third component is set to positive zero. The squared length is computed as the first square plus the second square, then plus the third square, with binary32 boundaries. Only an ordered result strictly greater than the stored 0.01 threshold invokes normalization. Normalization recomputes that binary32 squared length, widens it for the imported precise square root, narrows the returned binary64 root to binary32, divides binary32 one by that root, and multiplies each component by the resulting binary32 reciprocal. No normalization is performed for a squared length equal to or below the threshold, or for an unordered comparison.

After that optional normalization, the third component is replaced with binary32 0.25. The vector is then multiplied component by component, in first/second/third order, by the binary32 square root of the ratio between the attacker's first size component at `+0x80` and the target's first size component at `+0x80`. That ratio is divided in binary32, widened for the imported square root, and the returned root is narrowed to binary32. A second component-by-component multiplication applies the scalar obtained from RVA `0x044860` for this attacker and action. Both vector-scaling stages store every component as binary32. No zero-divisor, negative-ratio or finite-value guard is added by this construction. A zero action scalar does not skip the preceding ratio or square-root operations. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00614441–0x00614529`; helper bodies at RVAs `0x02C7A0`, `0x02C4A0`, `0x024860`, `0x0240F0`, `0x024B70` and `0x012780`.]

RVA `0x044860` first returns zero for numeric roles 3 and 5, regardless of action. For other roles it supplies:

| Actions | Vector-strength scalar |
| --- | --- |
| 30–33 | Zero |
| 1, 2, 9, 13, 14, 15, 60, 61 | Six divided by the binary32 speed result |
| 3, 4, 6, 7, 18, 19, 62 | Three divided by the speed result |
| 65, 66 | Ten divided by the speed result |
| 5 | Four |
| 11, 87 | Ten |
| 17, 20 | Six |
| 54 | Five |
| 21, 88 | The skill-derived scalar described below |
| All remaining actions | Two divided by the speed result |

For actions 21 and 88, the helper queries the action-21 skill coefficient through RVA `0x03ED60`, with the second argument minus one. The action mapping selects signed skill rank at creature `+0x1150`. If that rank is zero and the numeric role is nonzero, the effective integer rank becomes signed level divided by two toward zero, plus the unsigned rank byte, plus one, using native 32-bit integer arithmetic. After conversion to binary32, the rank is multiplied by 0.1, one is added, one is divided by that sum, and that quotient is subtracted from one. The resulting binary32 skill coefficient is multiplied by 30, then five is added. A zero, negative or otherwise unusual stored skill rank is not given an extra clamp by these scalar routines. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00444860–0x004449FD` and `0x0043ED60–0x0043EDDB`; original dispatch tables at RVAs `0x044A00`, `0x044A38`; `decompiled/client/functions/0043c9c0.c`.]

## Combo-dependent speed used by the vector scalar

RVA `0x047700` starts with binary32 one for role zero. For a nonzero role, it converts the unsigned rank byte to binary32, multiplies by 0.0625 and adds 0.75.

Class 1 with specialization zero adds half of a combo fraction to that starting speed. Class 3 with specialization one adds the full combo fraction. Other class/specialization combinations receive neither addition here. The fraction uses the signed 32-bit combo counter at `+0x70` divided by the signed 32-bit cap returned by RVA `0x044CB0`, after both integers are converted to binary32. Only an ordered fraction greater than one is replaced by one; there is no lower clamp. When the first comparison does not take that upper-clamp branch, the native routine calls the cap helper again and recomputes the fraction. That repeated query and its conversions remain part of the observed order.

The cap helper multiplies binary32 `+0x17C` by 60 for a category-3 record at `+0xAA8` with subtype 6; by 80 for subtype 8; or by 20 for subtypes 11, 12, 15, 16 and 17. Other category-3 subtypes use 50. If the `+0xAA8` record is not category 3, flag bit 16 at `+0x7E` selects 20, otherwise 50. The product is converted to a signed 32-bit integer by native truncation toward zero. No positive minimum is applied to that cap before the subsequent division.

The scalar haste/item result from RVA `0x0478B0` is narrowed to binary32 and added to the speed accumulated so far. If valid status-list records contain status identifier 12, the total is multiplied by a skill-shaped factor. Its rank is signed `+0x1158`, with the same zero-rank/nonzero-role level-half-plus-rank-plus-one fallback described above. The binary32 operations multiply rank by 0.1, add one, divide one by that sum, subtract that quotient from one, add one, and multiply the resulting factor by the accumulated speed. The returned speed is a stored binary32 value. These combo operations affect the vector-strength divisions; the principal hit block has no separate direct combo-counter damage multiplication. [Evidence: `decompiled/client/assembly.txt`, VAs `0x00447700–0x004478AA` and `0x00444CB0–0x00444D28`; `decompiled/client/functions/00447700.c` and `00444cb0.c`.]

## Bookkeeping and supplied generic-hit arguments

After constructing damage, the caller examines the attacker hit collection, inserts the candidate target's eight-byte identifier, and sets attacker byte `+0x13B8` to one before calling the generic hit routine. The bookkeeping occurs before the resolver can accept or reject the hit. A temporary Boolean relating collection emptiness and signed `+0x12C` is also prepared, but it is not one of the arguments supplied at this call. This contract does not assign its later use to the input payload. [Evidence: `decompiled/client/assembly.txt`, VAs `0x0061469C–0x006146E9`; original helper at RVA `0x077230` tests the collection's stored count for zero.]

At the call to RVA `0x196D30`, ECX supplies the world-update object. The arguments below are listed in stack argument order as observed by that callee, excluding ECX:

| Argument position | Supplied value |
| --- | --- |
| 1 | Candidate target creature pointer |
| 2 | Attacker creature pointer |
| 3 | Constructed binary32 damage, including the conditional critical doubling |
| 4 | Critical decision, conveyed as a one-byte Boolean in the low byte of its stack word |
| 5 | Earlier charge-related Boolean, conveyed in the low byte of its stack word |
| 6 | Current raw binary32 attacker charge at `+0x13B4`, read at argument construction |
| 7 | Pointer to attacker position at `+0x10` |
| 8 | Pointer to the constructed three-binary32 hit vector |
| 9 | The enclosing world-update call's forwarded second argument; its object semantics remain outside this scope |
| 10 | Address of the caller's existing local scratch object; its downstream use remains outside this scope |
| 11 | Magic Boolean, true only for actions 30, 31, 32 and 33 |
| 12 | Current action byte zero-extended to a 32-bit argument |
| 13 | Literal zero |
| 14 | Literal one |

The critical and charge Boolean locals are written as bytes before their containing words are pushed. The false magic case clears only CL before pushing ECX; the true case supplies the full integer one. The Boolean contracts concern the low byte; this caller does not establish zero-filled upper bytes for every false/true Boolean stack word. The action argument is explicitly zero-extended. Arguments 13 and 14 are full immediate zero/one words. No further meaning for those terminal controls is inferred from the resolver. [Evidence: `decompiled/client/assembly.txt`, VAs `0x006146F0–0x0061476B`; only the original call signature at the beginning of `decompiled/client/functions/00596d30.c` was consulted to bound the scalar/byte argument boundary, without auditing its resolution body.]

## Remaining limits

This contract closes the caller's branch memberships, selected-equipment order, action coefficients, charge multiplication, critical-versus-latched decision, visible random-draw order and conversions, combo-dependent vector scalar, vector construction and supplied argument slots. It does not close the complete item-damage, critical-stat or haste formulas; all upstream resource-cost/timing behavior; the internal semantics of forwarded context/scratch objects; downstream hit resolution; post-hit resource gains, effects, health writes or additional RNG draws.

Imported precise power and square-root implementations, CRT `rand` state/algorithm and allocation internals were not inspected. The constants, call ordering and narrowing boundaries are original caller evidence; imported last-bit math results and behavior under unusual runtime values remain subject to the native runtime. The examined code does not establish one universal ambient MXCSR/x87 control state. Division by zero, nonfinite values, NaNs, integer-conversion overflow, exception flags and traps therefore retain native imported-math and ambient-control limits. No replacement behavior or default clamp is supplied for those limits.
