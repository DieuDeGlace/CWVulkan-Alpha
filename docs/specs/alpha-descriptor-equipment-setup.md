# Alpha descriptor equipment and zero-option setup

Status: FINAL / STABLE / HELD after complete author readback.

This original-source-only contract covers client routine RVA `0x1C7730` through its ordinary returns, including early classification exceptions, equipment writes, action/configuration appends, random calls and the zero-option numeric phase. The supported image is `binary/Cube.exe`, base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. Paths are relative to `disassembled alpha cube world`; bare filenames are under `decompiled/client/functions`. Original file `005c7730.c`, native instructions `0x1C7730–0x1C8262`, image-resident switch tables and literal bytes supply the evidence. No other contract, reconstruction, replacement implementation or runtime observation is evidence here.

## Inputs, widths and whole-routine precedence

D denotes the descriptor pointer passed in the first stack argument; the second stack argument occupies four bytes and supplies the setup option. Only that option's low byte controls whether the later numeric phase is entered. A full argument such as `0x00000100` takes the same numeric-phase admission as zero; any nonzero low byte bypasses that phase. The body does not use an incoming receiver as an object input. The item predicate in the numeric phase receives D's item at `+0x8C8`; no world/context dependency is established by that call.

The initial flag source is D's 16-bit word `+0x7A`. Mask `0x0040` returns before all equipment, action, level, numeric and random writes described here. Next, classification byte `+0x30` values `0x80..0x89` take the special return described below. Neither role, kind nor setup argument cancels that classification exception. All remaining classifications take the generic route. Generic equipment admission next tests word mask `0x0018`, then exact full 32-bit role `+0x28` equal to 6, then full 32-bit kind `+0x2C`. These tests must not be replaced with narrowed role/kind comparisons. [Evidence: native RVAs `0x1C7758–0x1C7829`.]

Classification `+0x30` and its companion `+0x31` are bytes. Kind `+0x2C` and role `+0x28` are read at full 32-bit width. The source for every item-level write is read as a 16-bit word at `+0x34`, without clamping or signed conversion; no width or interpretation for adjacent source bytes is needed here. All action/candidate elements and mode `+0x10B8` are full 32-bit values. The routine contains no direct write to D's flag word, kind, role or source level.

## Classification exception and generic clearing

For classification bytes `0x80..0x89`, item word D `+0x350` becomes `0x0004`, clearing its subtype byte `+0x351`, and item byte `+0x35D` becomes 6. Classification `0x88` then consumes one random result, takes its signed remainder by 5, adds 2 with byte wrap and writes `+0x351`; the ordinary nonnegative results give subtypes 2 through 6. The other nine classifications consume no draw here. All ten then store, in order, binary32 bits `0x3DCCCCCD` at `+0xF60`, `0x3E99999A` at `+0xF5C`, and `0x41200000` at `+0xF64`, then `+0xF68`. These represent the original rounded 0.1 and 0.3 literals, and exact 10. `+0xF58`, hand levels and all unlisted item/action/configuration fields are retained. This return bypasses generic clearing, the numeric phase, mask `0x0200` processing and the six-draw tail. [Evidence: native RVAs `0x1C81EA–0x1C8262`.]

The generic route initially clears only the following item headers, in this order: 16-bit word `+0x7B0`; byte `+0x8C8`; byte `+0x8C9`; then 16-bit words `+0x698`, `+0x350`, `+0x468`, `+0x580` and `+0x238`. It does not construct or zero whole item records. In particular, the item records at `+0x9E0` and `+0xAF8` are not initially cleared. The unlisted seeds, levels, material bytes, payloads and other item members retain their incoming contents unless a later listed write reaches them. [Evidence: native RVAs `0x1C77C1–0x1C7807`.]

Below, hand item U begins at D `+0x8C8`, and hand item V begins at D `+0x7B0`. Within either item, type and subtype are bytes `+0/+1`, the additional byte used below is `+0x0D`, the level is a 16-bit word at `+0x10`, and the raw random member is a 32-bit value at `+4`. A word header write replaces type and subtype together. The initial generic clear therefore leaves U and V type/subtype zero but retains their other members.

## Fixed kind/role/flag equipment routes

All rows below follow the generic clear. The first matching admission takes precedence; these rows bypass randomized classification and its optional equipment draws. Except where listed, the incoming classification/companion bytes and item members are retained. Action lists are appended to, never cleared by this routine.

| Admission | Action/configuration effects | Hand/header effects after the initial clear |
| --- | --- | --- |
| Mask `0x0018` is nonzero | None in this equipment stage. | V's word header is cleared again, then U type and U subtype are each cleared again. |
| Mask `0x0018` clear; full role is 6 | None in this equipment stage. | No additional hand write before the common level copies. |
| Other admitted role; kind `0x76` | None in this equipment stage. | Repeats the V-word/U-type/U-subtype clear used by mask `0x0018`. |
| Kind `0x75` | Appends `0x5C` to D `+0x10AC`; stores mode `+0x10B8` as 0; appends `0x11`, then `0x60`, to D `+0x10BC`. | No additional hand header write. |
| Kind `0x6C` or `0x72` | Appends `0x5B`, then `0x5D`, to `+0x10AC`. | U type becomes 3, U subtype `0x11`, U byte `+0x0D` becomes 2, then V's word header becomes zero. |
| Kind `0x2E` | No action append at this equipment stage. | Same hand writes as `0x6C/0x72`. |
| Kind `0x73` | Appends `0x56`, then `0x5D`, to `+0x10AC`. | In store order: U subtype 4, U type 3, U byte `+0x0D` 1, V word header `0x0403`, V byte `+0x0D` 1. |
| Kind `0x6D` | Appends `0x56`, `0x5D`, then `0x5B`, to `+0x10AC`. | In store order: U subtype 2, U type 3, U byte `+0x0D` 7, V word header `0x0203`, V byte `+0x0D` 7. |
| Kind `0x51` | Appends `0x57` to `+0x10AC`. | U type 3, subtype `0x0C`, byte `+0x0D` `0x0B`; V word header `0x0C03`, then byte `+0x0D` `0x0B`. |
| Kind `0x52` | None at this equipment stage. | U type 3, U byte `+0x0D` 1, V word header `0x0003`, V byte `+0x0D` 1, then U subtype zero. |
| Kind `0x2F`, `0x6F`, `0x71` or `0x70` | None at this equipment stage. | No additional hand write before level copies. |
| Kind `0x2D` or `0x2B` | Appends `0x5F` to `+0x10AC`, then writes companion byte `+0x31` as 2. | U type becomes 3; one draw supplies a signed remainder by 2, added to 10 with byte wrap for U subtype; then U byte `+0x0D` becomes 2 and V word header becomes zero. |
| Every other full kind | The randomized classification route below. | The randomized classification route below. |

The action/configuration writes in each row precede its hand writes. The rows with no subtype/material write do not repair stale retained fields just because the type is zero. Kind and role exceptions above concern this equipment stage only: the common level copies, optional zero-low-byte numeric phase and six-draw tail still follow. [Evidence: native RVAs `0x1C7807–0x1C7A08`, `0x1C7D7F–0x1C7E1F`.]

## Randomized classification and hand outcomes

The remaining-kind route consumes a draw first, takes the signed remainder by 4, adds 1 with byte wrap and writes classification `+0x30`. The full-kind comparison with `0x61` is evaluated before that store; equality then causes a second store replacing the classification with 4. Its initial classification draw is still consumed. Dispatch uses the resulting unsigned byte and recognizes only 1 through 4. Every recognized classification consumes its own next draw for the signed remainder by 2, stored as companion byte `+0x31`, before the hand-selection draw. An out-of-range resulting classification retains the old companion byte and skips the class-specific hand writes, then still reaches the optional equipment gate below. [Evidence: native RVAs `0x1C7A08–0x1C7A3A`; image table `0x1C8264`.]

In the following table, each remainder comes from the next draw in the stated order. The table describes the exact signed selections; ordinary nonnegative random results use the listed 0/1/2 outcomes. A negative remainder other than the explicitly accepted cases takes the listed fallback rather than being converted to an unsigned selection.

| Resulting classification | Next hand draw and further draws | Observable hand writes, in native order |
| --- | --- | --- |
| 1 | Signed remainder by 3 equals 0; then one further draw, also signed remainder by 3. | U type 3 before that further draw; then U byte `+0x0D` 1, V word header zero, U subtype becomes the further remainder plus 15 with byte wrap. V byte `+0x0D` is retained. |
| 1 | Hand remainder equals 1; then separate further draws for U subtype and V subtype, each signed remainder by 3. | U type 3 before the U draw; then U byte `+0x0D` 1, V type 3, U subtype from its draw; after the V draw, V subtype from its result, then V byte `+0x0D` 1. |
| 1 | Hand remainder equals 2; then one further signed-remainder-by-3 draw. | U type 3 before that draw; then U byte `+0x0D` 1, V word header `0x0D03`, U subtype from the further remainder, then V byte `+0x0D` 1. |
| 1 | Other hand remainder | No hand write beyond the initial clear. |
| 2 | One signed-remainder-by-3 draw after companion selection. | U type 3 before this draw; then U byte `+0x0D` 2 and U subtype becomes the remainder plus 6 with byte wrap. V's cleared header and retained material remain. |
| 3 | One signed-remainder-by-3 draw after companion selection. | Remainder 0 writes U subtype 10; remainder 1 writes U subtype 11. Either then writes U type 3, U byte `+0x0D` 2, and V word header zero. Remainder 2 instead writes U type 3, subtype 12, U byte `+0x0D` 11, V word header `0x0C03`, V byte `+0x0D` 11. Other remainders leave both initially cleared headers unchanged. |
| 4 | One signed-remainder-by-3 draw after companion selection. | U type 3 precedes the draw; U byte `+0x0D` becomes 1; U subtype becomes the remainder plus 3 with byte wrap. If the resulting subtype is 5, V word header and V byte `+0x0D` both become zero. Otherwise V type becomes 3, V subtype copies U's resulting subtype, then V byte `+0x0D` becomes 1. |

These routes do not append class-derived action values. For example, class-3 subtype 10 and subtype 11 do not share a second random subtype draw, and class-1 hand remainder 1 consumes two further subtype draws while its 0 and 2 cases consume one. The complete distinction matters before all subsequent optional and tail draws. [Evidence: native RVAs `0x1C7A3A–0x1C7BDD`.]

## Optional equipment on the randomized route only

Only the remaining-kind randomized route reaches the byte-mask-`0x20` test at D `+0x7A`. A set bit skips all writes and draws in this section. With it clear, the routine derives a byte value from U's current subtype: value 25 for subtypes 10/11/12, value 26 for 6/7/8, value 27 for 3/4/5, and value 1 for every other byte. It writes D byte `+0x350` as type 4, copies low level `+0x34` to word `+0x360`, then writes the derived byte to `+0x35D`. Its initially cleared subtype byte `+0x351` remains zero. [Evidence: native RVAs `0x1C7BDD–0x1C7C47`.]

The following gates then consume draws in exactly this order. Every gate draw occurs; a failed gate skips that row's writes and conditional extra draw, then proceeds to the next gate.

| Gate draw | Admission | Writes and conditional extra draw |
| --- | --- | --- |
| First | Signed remainder by 2 is nonzero. | Copies low level to word `+0x6A8`, writes type byte `+0x698` as 7, then derived byte at `+0x6A5`. |
| Second | Signed remainder by 2 is nonzero. | Copies low level to word `+0x590`, writes type byte `+0x580` as 5, then derived byte at `+0x58D`. |
| Third | Signed remainder by 2 is nonzero. | Copies low level to word `+0x478`, writes type byte `+0x468` as 6, then derived byte at `+0x475`. |
| Fourth | Signed remainder by 10 is zero. | Writes type byte `+0x238` as 8, copies low level to word `+0x248`, then consumes a separate parity draw. Its signed remainder by 2 being zero stores byte 12 at `+0x245`; any nonzero remainder stores 11. |
| Fifth | Signed remainder by 10 is zero. | Writes type byte `+0xAF8` as 9, copies low level to word `+0xB08`, then consumes the same form of conditional parity draw for byte `+0xB05`, giving 12 for zero and 11 otherwise. |
| Sixth | Signed remainder by 10 is zero. | Writes type byte `+0x9E0` as 9, copies low level to word `+0x9F0`, then consumes the same form of conditional parity draw for byte `+0x9ED`. |

The first three successful rows do not consume another draw. The final three rows consume their extra draw only on success, before the next gate draw. A failed early row leaves its type/subtype initially cleared but preserves the old unlisted level/material fields. A failed `+0xAF8` or `+0x9E0` row leaves the entire incoming record unchanged, because those records were never initially cleared. Their successful rows also retain subtype bytes `+0xAF9/+0x9E1` and all other unlisted fields. This section contributes six through nine direct draws. [Evidence: native RVAs `0x1C7C47–0x1C7D7A`.]

## Common hand levels and low-byte argument gate

Every generic route next copies the same low 16 bits of D `+0x34` first to U level word `+0x8D8`, then V level word `+0x7C0`. These copies occur even for cleared/inactive hand types, role 6, mask `0x0018`, and every fixed kind. There is no sign extension, positive-level admission or maximum clamp in either store. The low setup byte is compared before those writes, but the branch that bypasses the numeric phase is taken only after both writes. A nonzero low byte proceeds directly to the six-draw tail. A zero low byte admits the numeric table and mask-`0x0200` phase below. [Evidence: native RVAs `0x1C7E1F–0x1C7E3B`.]

## Zero-low-byte kind-specific numeric phase

All listed numeric fields are binary32 storage at D `+0xF58`, `+0xF5C`, `+0xF60`, `+0xF64`, `+0xF68`. The table contains every nondefault kind result from the original full-width switch; a field omitted from a row retains its incoming value until the later mask phase. Listed literal values except the earlier 0.1/0.3 exception are exactly representable in binary32. The action column appends to `+0x10AC` at the placement stated in its row; where no intervening placement is stated, the append follows that row's numeric stores.

| Full kind | Numeric writes, in order | Other effects |
| --- | --- | --- |
| `0x11`, `0x5E`, `0x61` | `+0xF58` becomes 400; `+0xF60` 1.5; `+0xF64` 3; `+0xF68` 3. | Appends `0x5D`. |
| `0x15` | `+0xF58` 250; `+0xF60` 2; after its action append, `+0xF68` 3, then `+0xF64` 3. | Appends `0x45` between the first two and final two stores. |
| `0x19` | `+0xF58` 200; `+0xF60` 0.5; `+0xF68` 3; `+0xF64` 3. | None. |
| `0x2E`, `0x52` | `+0xF58` 250; `+0xF64` 3; `+0xF68` 3. | Appends `0x56`; `+0xF60` is retained. |
| `0x2F`, `0x58` | Reads old `+0xF5C` and forms its binary32 product with 0.5; stores `+0xF58` 250, `+0xF60` 3, then that product at `+0xF5C`, then `+0xF68` 3 and `+0xF64` 3. | None. |
| `0x51` | `+0xF58` 250; `+0xF64` 3; `+0xF68` 3. | Appends `0x5C`; `+0xF60` is retained. |
| `0x55` | `+0xF58` 1000; `+0xF60` 2; `+0xF68` 3; `+0xF64` 3. | None. |
| `0x56` | No numeric member write. | Writes the 16-bit classification/companion word at `+0x30` as `0x0103`, after the equipment stage has already completed. |
| `0x6B`, `0x6C`, `0x6D`, `0x6F`, `0x70`, `0x71`, `0x72`, `0x73`, `0x74`, `0x76` | `+0xF58` 2500; `+0xF60` 6; `+0xF64` 5; `+0xF68` 5; invokes the U-item predicate below; then `+0xF5C` becomes 0.75 for true or 0.5 for false. | No action append in this numeric row. Kind `0x6E` is not in this set. |
| `0x75` | `+0xF58` 2500; `+0xF60` 5; `+0xF64` 4; `+0xF68` 5. | `+0xF5C` is retained. |
| `0x77` | `+0xF5C` 0.5 first; `+0xF58` 2500; `+0xF60` 6; `+0xF64` 10; `+0xF68` 10. | None. |
| Every other full kind, including values outside `0x11..0x77` | No kind-specific numeric write. | Continues to the mask phase with retained numeric members. |

The full image switch mapping was checked at RVA `0x1C8274`, with byte dispatch table `0x1C82A4`. The numeric writes are at `0x1C7E60–0x1C8035`. In particular, the equipment-stage role/mask bypasses do not cancel these rows. A repeated call can repeat appends and multiplications; this routine does not make setup idempotent.

The predicate at `0x044820` is fully bounded here. Its receiver is U at D `+0x8C8`. It returns true only when U's type byte is 3 and its subtype is one of `0x0F`, `0x10`, `0x11`, 5, 10, 11, `0x12`, 8, 6 or 7, checked in that order. It reads no level, descriptor role or world state, performs no write and has no nested call or random draw. Thus the numeric group's `+0xF5C` is based on the hand state already produced above. No unclosed external stat query is substituted for this predicate. [Evidence: `00444820.c`; native RVAs `0x044820–0x044855`, caller `0x1C7E60–0x1C7EB4`.]

## Zero-low-byte mask-0x0200 phase

After the numeric switch, mask `0x0200` in D word `+0x7A` is tested. If clear, the routine proceeds to the common tail. If set, it performs these binary32 operations in order, regardless of role 6, mask `0x0018`, mask `0x20`, or whether the kind matched a numeric row:

1. Forms `+0xF58` multiplied by 15, compares that rounded product with 10000, then stores the product. An ordered-greater result additionally stores literal 10000. Equality does not cause this second store; there is no lower clamp.
2. Stores `+0xF60` multiplied by 3.
3. Stores `+0xF5C` multiplied by 0.75.
4. Stores `+0xF64` plus 2, then `+0xF68` plus 2.

Every arithmetic result rounds to binary32 before its store. The constants are 15 (`0x41700000`), 10000 (`0x461C4000`), 3 (`0x40400000`), 0.75 (`0x3F400000`) and 2 (`0x40000000`). A prior kind-specific multiplication by 0.5 remains a separate earlier binary32 operation and must not be folded into this later multiplication. Unordered exceptional comparison follows the native unordered branch without inventing a finite repair; exception traps and nonstandard floating controls are outside this ordinary relation. [Evidence: native RVAs `0x1C8035–0x1C80DC`; image constants at `0x2FCE38`, `0x2FD908`, `0x2FCD70`, `0x2FCDC4`, `0x345E10`, with the 0.5 constant at `0x345DA0`.]

The routine then builds a temporary local four-byte-element collection containing `0x5C`, `0x5D`, `0x56`, `0x59`, `0x57`, in that order. These five construction appends are not five appends to D's action list. One imported random result is treated as unsigned and divided by the temporary collection's actual logical length; its remainder selects one element, which is appended to D `+0x10AC`. On the ordinary successful route the length is exactly five. No existing descriptor actions are removed, and a chosen value can duplicate an earlier action append.

A second random result then supplies a signed remainder by 3, stored full-width at D `+0x10B8`. D `+0x10BC` receives appends `0x11`, then `0x60`. The temporary local collection is destroyed after those appends and before the six-draw tail. Its cleanup does not clear either descriptor collection. Exact full kind `0x75` can therefore have its earlier mode zero overwritten and receive a second `0x11/0x60` pair on this zero-option/mask route. [Evidence: native RVAs `0x1C80DC–0x1C81A6`, original collection helpers `0066add0.c`, `005fb860.c`.]

## Tail, draw accounting and remaining limits

Every generic ordinary route finishes with six imported random calls, storing each full 32-bit result before taking the next draw. The destinations are, in order, D `+0x7B4`, `+0x8CC`, `+0x69C`, `+0x354`, `+0x584`, `+0x46C`. These stores occur even when the corresponding item's type is zero. They do not cover the random-member locations of the accessory items at `+0x238`, `+0xAF8` or `+0x9E0`. No item level or subtype is recomputed from these tail values. [Evidence: native RVAs `0x1C81A6–0x1C81D6`.]

For ordinary nonnegative random results, the direct draw counts are compositional: fixed equipment routes above contribute zero, except `0x2B/0x2D` contribute one; randomized classification 1 contributes four draws in hand cases 0/2 or five in hand case 1, while classifications 2/3/4 contribute three including the initial classification draw. A forced classification 4 for full kind `0x61` still contributes three. The optional equipment phase adds six through nine only when reached with mask `0x20` clear. The zero-option mask-`0x0200` phase adds two. Every generic path adds the final six. The special classification returns instead contribute one for `0x88`, zero for the other nine; the earliest mask-`0x0040` return contributes zero.

The signed remainders, byte additions/narrowing and unsigned boss-pool selection above specify the integer distinctions directly. This contract does not define the random generator algorithm or assert interchangeable signed/unsigned selection for arbitrary bit patterns. It also does not remove an apparently discarded classification draw, failed optional gate draw or tail draw for a type-zero item.

The supported image's random target is `MSVCR110.dll` import `rand` at IAT RVA `0x2FC468`; the generic route caches that same target and the special-`0x88` call reaches it directly. There is no direct `srand` call in this routine. Reached game-side nested helpers are the item predicate, four-byte collection append/capacity helpers, and temporary collection cleanup. Their inspected bodies add no random or seed calls. The capacity path uses ordinary allocation, movement and release of four-byte elements; it supplies no game-stat generator or gameplay callback. Allocation, imported-runtime behavior, exception cleanup and external allocator customization remain outside the ordinary successful-call relation. [Evidence: files `0066add0.c`, `0063dc90.c`, `0063da20.c`, `005fb860.c`; PE import identity and native call sites.]

All item fields not explicitly written retain their incoming values on the reached ordinary route; this is not a claim that the incoming descriptor was freshly constructed. The routine does not clear action or candidate collections, normalize item payloads, seed the random stream, assign gameplay labels to numeric kinds/classes, or guarantee a useful return scalar for all early routes. Caller admission, source-field initialization, earlier seed lifetime, full item stat semantics, exceptional storage/arithmetic states, concurrency and a server counterpart are not established here. Earlier held artifacts remain independent and unchanged.
