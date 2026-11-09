# Alpha creature yaw light gates

This is an original-client contract for four creature-dependent terrain-light sites in renderer RVA `0x0AC260`. The evidence root is `disassembled alpha cube world`. The supported `binary/Cube.exe` is 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x00400000`. Every address below is an RVA. Supporting original sources are `decompiled/client/assembly.txt`, the image's bytes and import table, and the individually identified function files cited below. No reconstruction, other project contract, replacement implementation or runtime artifact was read for this audit.

The scope is the three source gates at `0x0AF632–0x0AFA87`, the action-light site at `0x0AFFA9–0x0B0184`, their local attachment calculations, and the threshold/stat dependencies actually reachable from the action-light call with argument -1. Other light sites, source-state producers and the complete renderer caller admission remain outside this contract.

## Admission, outputs and relative order

These sites belong to the traversal of the client creature container at `+0x2E8`. The immediately enclosing checks require a nonnull creature pointer and ordinary positive binary32 health at creature `+0x16C`. They do not select only the controlled creature. None of the four local sites calls the renderer's view/distance helper. This does not establish the absence of restrictions imposed by the surrounding renderer or its callers. Evidence: `binary/Cube.exe`, `0x0AF5C5–0x0AF632`, and the four scoped site bodies.

For each admitted creature, the first three sites are independent and considered in the following order. Their positions all use the first attachment described below.

| Site | Exact local gate | Radius | RGB before any downstream shading |
|---|---|---|---|
| First, append `0x0AF782` | Byte `+0xAA9` equals decimal 20, or byte `+0x991` equals 20 | Binary32 32 | Binary32 literals `(1.2, 1, 0.5)` |
| Second, append `0x0AF932` | Unsigned 16-bit field `+0x124` contains bit `0x0200`, and byte `+0xDF0` equals decimal 24 | Unsigned byte n at `+0xDFC` is zero-extended, multiplied by integer 8 and increased by integer 10; that signed 32-bit result is converted to binary32 | `(0.6, 0.5, 0.4)`, each multiplied by the binary32 result of n converted to binary32, multiplied by `0.25`, then increased by `0.75` |
| Third, append `0x0AFA87` | Resident lookup `0x043F10` with type argument 9 returns nonnull | Binary32 16 | Binary32 literals `(0.6, 0.4, 0)` |

The first site tests `+0xAA9` before `+0x991`; equality of the first avoids reading the second for this gate. Both bytes being 20 still produce only one candidate at this site. It does not test the preceding item-category bytes `+0xAA8` or `+0x990`. The second site tests the word bit before testing `+0xDF0`; it does not add a class, subtype, positive-n or resource test. n is read separately for radius and color. For its full unsigned-byte range, the integer radius calculation is in range and exactly convertible to binary32. No common clamp is applied to its radius or RGB. These gates are not alternatives to the resident-type-9 site. Evidence: `binary/Cube.exe`, `0x0AF632–0x0AF942`.

The type lookup walks the list whose head is at creature `+0x1178`, reads each node's unsigned type byte at node `+0x08`, and returns the first matching payload or null at the sentinel. This lookup checks neither a timer nor the buff's source identifier, amount or other payload fields. It does not erase or consume a matching record. The third light therefore depends on resident presence at this point; no independent expiry check is performed by this light gate. Evidence: original helper `0x043F10`, `decompiled/client/functions/00443f10.c`, renderer call `0x0AF93B`.

The action-light site comes later in this same creature traversal, after additional light sites that are not characterized here. It admits action-byte values 28, 36, 94 and 95 without calling the threshold helper or testing the action clock. For action-byte values 37, 38, 43, 44, 87, 88, 89 and 92, it calls `0x03CAA0` once with the same creature and signed 32-bit argument -1. Its signed 32-bit clock at `+0x6C` must then be strictly less than the helper's signed 32-bit result. Equality rejects, as does a greater clock. Other action-byte values produce no candidate at this site. There is no separate zero-clock, positive-clock or nonnegative-threshold check. In particular, a zero clock rejects when the returned threshold is zero; a negative clock is evaluated by the same signed comparison. Evidence: `binary/Cube.exe`, `0x0AFFA9–0x0B000B`.

An admitted action light uses the second attachment below, radius 20 and base RGB `(0.3, 0.2, 0.1)`. Its scalar color modulation is obtained from the signed 32-bit action clock converted to binary32, multiplied by binary32 `0.01`, and supplied to cosine adapter `0x00E420`. That adapter promotes the already rounded input to binary64, calls the imported precise cosine, rounds its result to binary32, and returns that value through the floating return path. The renderer stores it as binary32, multiplies by `0.1`, then adds 1. The three base RGB components are multiplied separately by the completed scalar. The scalar is not applied to radius or position, and no RGB clamp occurs here. Candidate append is at `0x0B017F`. Evidence: `binary/Cube.exe`, `0x0B00CD–0x0B017F`, `0x00E420–0x00E43D`; `decompiled/client/functions/0040e420.c` and `004252f0.c`.

## Threshold type, action selection and numeric boundaries

Helper `0x03CAA0` receives a 32-bit action argument. A signed-negative argument makes it reread creature `+0x68` as an unsigned byte and select that action's branch. Thus -1 is a selection sentinel, not a numeric duration or scale. The helper does not use the action clock as an arithmetic operand. The renderer's membership test and this reread are distinct reads; behavior under concurrent action changes is not closed by the ordinary unchanged-input relation below.

For the eight conditional actions at this call site, the original branch table selects these binary32 numerators:

| Current action reread by the helper | Numerator and extra branch |
|---|---|
| 37, 43, 89 | 600 if byte `+0xAA9` equals 12; otherwise 1200. This branch does not test byte `+0xAA8`. |
| 38, 44 | 500 |
| 87 | 5000 |
| 88, 92 | 400, through the helper's default branch |

Every one of these branches calls stat helper `0x047700` once. Its floating return is stored to binary32. That rounded value is multiplied by creature binary32 field `+0x17C`, with a binary32 result. The selected numerator is divided by that result in binary32. The quotient is converted to signed 32-bit by truncation toward zero and returned as an integer; the renderer compares the clock to this integer without converting the clock to floating point. The result is not rounded to the nearest integer, and the quotient is not retained as a fractional clock boundary. No lower or upper clamp is added by `0x03CAA0` before conversion. This ordinary relation requires finite arithmetic, a nonzero denominator and an in-range integer conversion. Other action cases of this general helper are outside the reached action-light subset; the four unconditional light actions do not borrow those cases. Evidence: `binary/Cube.exe`, branch table at `0x03CE24/0x03CEB0`, branches `0x03CAFD`, `0x03CB4B`, `0x03CBBC`, `0x03CDF8`, common result `0x03CE0D–0x03CE1F`; `decompiled/client/functions/0043caa0.c`. Numerator literals are at RVAs `0x2FCE70`, `0x2FCE74`, `0x2FD8EC`, `0x2FD8F8` and `0x2FD900`.

## Reached stat-helper dependencies

The following describes the ordinary numeric dependency returned by `0x047700` without assigning unproved gameplay names to its fields. Its arithmetic is binary32, including the additions between contributions. The final result is loaded from a binary32 local for its floating return, and the threshold helper stores it to binary32 again. The return is therefore a binary32 value carried through the original floating ABI, not an extended-precision accumulation that survives into threshold division.

The starting contribution is 1 when creature byte `+0x60` is zero. Otherwise it is unsigned byte `+0x1A8` converted to binary32, multiplied by exact `0.0625`, then increased by `0.75`.

Two specific byte pairs at `+0x140/+0x141` add a counter-ratio contribution before the item/stat contribution. Pair `(1,0)` adds half of the selected ratio; pair `(3,1)` adds the ratio without halving. The ratio uses signed 32-bit `+0x70` converted to binary32 divided by helper `0x044CB0`'s signed integer result converted to binary32. An ordinary first ratio greater than 1 is replaced by 1. For a ratio less than or equal to 1, including equality, `0x044CB0` is called a second time and the counter is reread to recompute the ratio; the first quotient is not reused. There is no lower clamp. Other byte pairs make neither addition. Evidence: `binary/Cube.exe`, `0x047700–0x047809`; `decompiled/client/functions/00447700.c`.

Helper `0x044CB0` selects an integer multiplier from the item bytes at `+0xAA8/+0xAA9`. Category byte 3 and subtype 6 selects 60; subtype 8 selects 80; subtypes 11, 12, 15, 16 and 17 select 20; other subtypes select 50. When category is not 3, low-byte bit `0x10` at `+0x7E` selects 20, otherwise 50. It multiplies creature binary32 `+0x17C` by that selected literal and truncates the binary32 product toward zero to signed 32-bit. It has no clamp. Consequently its integer truncation occurs before the ratio division in `0x047700`. Evidence: `binary/Cube.exe`, `0x044CB0–0x044D28`; `decompiled/client/functions/00444cb0.c`.

After any byte-pair contribution, `0x047700` always calls `0x0478B0`, stores the return to binary32, and adds it to the running contribution. That helper begins with a base derived from signed 32-bit creature `+0x190`. It converts the field to binary32, subtracts 1, multiplies by `0.05`, adds 1, divides binary32 1 by that result, subtracts the quotient from 1, then multiplies by 3. The completed binary32 value is promoted to binary64 as an exponent for the imported precise power routine with binary64 base 2. The power result is rounded to binary32. A second power call uses base 2 and an exponent formed by binary32 `0.25` multiplied by binary32 zero, then promoted to binary64; this call is present even though its ordinary exponent is zero. Its result is rounded to binary32 and multiplied by the first rounded power. A third power call uses binary64 base 2 and binary64 exponent 3; its result is rounded to binary32. The completed product is divided by this third rounded power, then multiplied by `0.1`. These intermediate roundings and three power calls are retained; the native path does not replace the last two powers with compile-time constants. Evidence: `binary/Cube.exe`, `0x0478B0–0x04797C`; `decompiled/client/functions/004478b0.c`.

The same helper then adds selected item contributions from `0x0C7C00` in the order below. A category byte mismatch skips that item call. Each called floating return is stored as binary32 before being added to the running binary32 sum.

| Order | Item begins at creature offset | Required first byte |
|---|---|---|
| 1 | `+0x990` | 3 |
| 2 | `+0xAA8` | 3 |
| 3 | `+0x530` | 4 |
| 4 | `+0x648` | 6 |
| 5 | `+0x760` | 5 |
| 6 | `+0x878` | 7 |
| 7 | `+0x418` | 8 |
| 8 | `+0xBC0` | 9 |
| 9 | `+0xCD8` | 9 |

Evidence: `binary/Cube.exe`, `0x047955–0x047AF9`; `decompiled/client/functions/004478b0.c`.

After this sum is returned and added, `0x047700` searches the resident list at `+0x1178` for its first type byte 12. Absence leaves the sum unchanged. For an ordinary valid matching node, it reads signed 32-bit creature `+0x1158`. Only when that value is zero and byte `+0x60` is nonzero does it instead form an integer from signed `+0x190` divided by 2 with truncation toward zero, plus unsigned byte `+0x1A8`, plus 1. The selected integer is converted to binary32, multiplied by `0.1` and increased by 1. The helper divides binary32 1 by that value, subtracts the quotient from 1, adds 1, and multiplies this completed factor by the running sum. It applies this factor once, not once per matching record, and does not read a matching buff's duration, source or amount. The native address guard rejects a matching payload address that would be null; ordinary valid list nodes do not encounter that guard. No resident record is removed. Evidence: `binary/Cube.exe`, `0x047818–0x0478AA`; `decompiled/client/functions/00447700.c`.

## Item scalar dependency reached by the stat helper

Helper `0x0C7C00` admits item category bytes 3 through 9 and otherwise returns zero. The category gates in the preceding table already select admitted categories. It starts a binary32 coefficient at `0.1`, replacing it with `0.2` for category 4 or category 3 with subtype byte in `{5,6,7,8,10,11,15,16,17,18}`.

Its other coefficient comes from unsigned 32-bit item `+0x04` modulo 21. This is unsigned integer remainder of stored data, not a random draw. The remainder is converted to binary64, has binary64 zero added, is rounded to binary32 and divided by binary32 20. Item byte `+0x0D` equal to 12 then increases it by binary32 1; other byte values do not.

The numeric scaling factor comes from helper `0x045FF0`. Its first argument is signed 16-bit item `+0x10` sign-extended and converted to binary32. Its second is unsigned byte `+0x0C` zero-extended to a 32-bit integer argument. For the first power, `0x045FF0` uses the same subtract-1, multiply-by-`0.05`, add-1, reciprocal, subtract-from-1 and multiply-by-3 exponent construction described for the creature base, now starting with this item argument. It promotes the resulting binary32 exponent to binary64, calls precise power with base 2, and rounds the result to binary32. The second exponent is the second integer argument converted to binary32 and multiplied by `0.25`, then promoted to binary64 for power with base 2. That result is rounded to binary32 and multiplied by the first rounded power. A third base-2, exponent-3 power is rounded to binary32, and the product is divided by it. The result is stored as binary32 and returned through the floating return path. Evidence: `binary/Cube.exe`, `0x045FF0–0x04609C`; `decompiled/client/functions/00445ff0.c`.

Back in `0x0C7C00`, the returned factor is stored to binary32, multiplied first by the selected `0.1`/`0.2` coefficient, then by the modulo-derived coefficient. An ordinary result below binary32 literal `0.0010000000474974513` is replaced by zero; equality survives. The surviving binary32 result is returned through the floating return path. Each item call reached from the category table therefore has three power calls, even if its final contribution becomes zero. Evidence: `binary/Cube.exe`, `0x0C7C00–0x0C7D12`; `decompiled/client/functions/004c7c00.c`. This closes its numeric dependence on stored item fields, but not the provenance or mutation policies of those fields.

## Local yaw calculations and precision

Each admitted site constructs its own binary32 identity matrix through `0x023E70`; these sites do not reuse a previously computed yaw result. The first three sites read angle `+0x30`, the third binary32 component of the vector beginning at `+0x28`. The action-light site instead reads `+0x137C`, the third component of the vector beginning at `+0x1374`.

Rotation helper `0x024610` multiplies the supplied angle by binary32 literal `0.01745329238474369` (bits `0x3C8EFA35`) before any promotion. It promotes that rounded angle to binary64 for precise cosine, rounds the returned cosine to binary32 and saves it, then promotes the same rounded angle for precise sine and rounds that result to binary32. Cosine therefore precedes sine. Matrix updates use separate binary32 multiplications, additions and subtractions; no fused multiply-add or binary64 matrix accumulation occurs in this path. From the identity input, the ordinary finite result is a Z rotation whose first horizontal column is rounded cosine then rounded sine, and whose second is negative rounded sine then rounded cosine. Evidence: `binary/Cube.exe`, `0x024610–0x024723`; `decompiled/client/functions/00424610.c`, `00423e70.c`.

For the first three sites, local X and Y are binary32 `0.5` and 1. Local Z is creature binary32 `+0x88` multiplied by binary32 `0.3` (bits `0x3E99999A`), rounded before transformation. Point helper `0x0248A0` uses the just-built matrix. For output X, the rotated Y product is added to the rotated X product in that order, followed by the Z contribution and translation. For output Y, the rotated X product is added to the rotated Y product, followed by Z and translation. Z similarly accumulates its X, Y and Z products, then translation. The helper separately forms the homogeneous denominator in X-product, Y-product, Z-product, constant order, divides binary32 1 by it, and multiplies each completed output numerator by that reciprocal. Every multiply, add and divide here is binary32. With this identity-derived finite rotation, the denominator is ordinarily 1 and translation is zero, but the native point helper still evaluates them. Thus the ordinary horizontal offset combines negative sine times local Y with cosine times local X for X, and sine times local X with cosine times local Y for Y; vertical input is carried through the Z rotation. Evidence: `binary/Cube.exe`, `0x0248A0–0x024984`, renderer first-site calculations `0x0AF648–0x0AF6F8`, second `0x0AF7AE–0x0AF862`, third `0x0AF948–0x0AFA01`; `decompiled/client/functions/004248a0.c`.

The action-light local vector is `(0,1,0)`. Vector helper `0x088E50` uses the newly built rotation but omits translation and the homogeneous reciprocal. Its X accumulation adds the rotated Y product to the rotated X product, then Z; its Y and Z accumulations add rotated X then rotated Y, then Z. Products and additions are separately binary32. For the ordinary finite identity-derived rotation and this vector, the output direction is negative rounded sine in X, rounded cosine in Y and zero in Z. It is not the first sites' `(0.5,1,scaled-Z)` offset. Evidence: `binary/Cube.exe`, `0x088E50–0x088EDD`, `0x0B000B–0x0B0084`; `decompiled/client/functions/00488e50.c`.

In all four sites, `0x02C460` converts the completed binary32 vector component by component in X, Y, Z order. It multiplies a component by binary32 65,536, stores that product as binary32, and converts it through `0x28D946` to signed 64-bit with ordinary in-range truncation toward zero. Only after this conversion is the offset added, as three 64-bit integer additions, to the creature's rendered fixed position at `+0x1350/+0x1358/+0x1360`. The rendered base is not first rounded to floating point. The main position beginning at `+0x10` is not substituted for that base. Evidence: `binary/Cube.exe`, `0x02C460–0x02C49D`, `0x28D946–0x28D9BA`; `decompiled/client/functions/0042c460.c` and `0042c800.c`.

The completed fixed position also supplies the candidate's distance priority. The original fixed dot helper divides each signed 64-bit product by 65,536 before summing; the squared-distance result relative to client fixed position `+0x140` is rounded to binary32 and multiplied by binary32 `1/65536`. At the action site, this priority is completed before the separate clock-dependent color cosine. Radius, RGB and priority do not feed back into the yaw angle. The completed temporary record is appended after these values are formed. Evidence: the four site bodies; original helpers `0x03AC20`, `0x03B4A0`, `0x0120F0`, `0x0C11F0`.

## Imported math, side effects and limits

The supported image identifies the following original imports and local thunks:

| Operation | Local thunk RVA | Import-address slot RVA | Imported identity |
|---|---|---|---|
| Precise cosine | `0x28D7A4` | `0x2FC444` | `MSVCR110.dll`, `_libm_sse2_cos_precise` |
| Precise sine | `0x28D9C2` | `0x2FC45C` | `MSVCR110.dll`, `_libm_sse2_sin_precise` |
| Precise power | `0x28D8D4` | `0x2FC454` | `MSVCR110.dll`, `_libm_sse2_pow_precise` |

Evidence: `binary/Cube.exe` import descriptors, name records and the three thunk bodies. This establishes the original imported identities and the caller's precision boundaries, not an independently audited result-bit contract for every installed runtime version or exceptional math input.

No random-number call occurs in the four scoped site bodies or the reached threshold, stat, item, transform and conversion helpers. In particular, the item modulo operation reads existing data; it does not advance a generator. Each admitted yaw calculation makes its own cosine call followed by sine. The action site makes an additional color cosine after placement and priority. Conditional action admission runs its stat/power dependency before those yaw calls; rejected conditional actions do not reach the yaw or color calls. The class-byte ratio branch can call `0x044CB0` once or twice as specified, and the selected item categories control the number of item/power calls. These call-count differences must not be replaced by a constant stat evaluation.

The inspected helpers write temporary matrices, outputs and locals, and the candidate append changes the temporary collection. They do not spend creature resources, advance the action clock, select a new action, consume resident type 9 or type 12, or emit a gameplay event at these light sites. This is a renderer read of state, not an input-start admission rule. The audit makes no claim that the entire surrounding renderer or creature update is free of random draws or other effects.

The closed arithmetic relations assume valid storage, ordinary finite values, in-range conversions and unchanged input fields during a local evaluation. Production and synchronization of the angle vectors, rendered position, creature fields, equipment records and buff list are unresolved dependencies. The audit does not close floating-control-state changes, NaN/unordered comparisons, infinities, subnormal handling, signed integer overflow, fixed-point overflow, zero denominators, out-of-range conversion behavior with different exception masks, malformed lists, runtime math error handling, all renderer/caller gates, downstream candidate sorting or shader lighting. Other action cases of `0x03CAA0` and gameplay meanings of the numeric fields are not inferred.

Status: FINAL / STABLE / HELD after complete author readback. All prior held artifacts remain unopened and unchanged for this scope.
