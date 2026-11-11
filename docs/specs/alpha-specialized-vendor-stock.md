# Original specialized descriptor stock

Status: FINAL / STABLE / HELD.

This independent source contract covers the original client descriptor routes assigning class byte `0x81` or `0x82`. The supported original is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, with preferred base `0x00400000`. Addresses are RVAs. Evidence filenames are relative to `Documents/disassembled alpha cube world`. Only the original image, its assembly listing and individually identified original decompilations were used; no earlier contract or reconstruction supplied evidence.

The bounded stock routines are `0x1C8390` and `0x1CA0A0`. Both build their own candidate pools directly, then select multiple stock entries from those pools. Neither calls the stock body `0x1C9B30` or the item selectors `0x1F51E0/0x1F8AD0`. Native instructions establish the pool contents, copies and removals even where the decompiled files mark container operations unreachable or collapse their pointer values. The contract assumes ordinary valid storage and successful reached services; wider generation and lifetime limits appear at the end. [Evidence: `binary/Cube.exe`, RVAs `0x1C8390–0x1C9B1E`, `0x1CA0A0–0x1CADD2`; `decompiled/client/functions/005c8390.c`, `005ca0a0.c`; `decompiled/client/assembly.txt`, corresponding function intervals.]

## Dispatch and descriptor publication

At `0x1E9672`, routine `0x1E4850` calls routine `0x1AF170` only on its observed branch where the second supplied object's 32-bit `+0x18` is 1 or 5. The two specialized routes below occur inside `0x1AF170` under the second object's `+0x18 == 1` branch. Its first supplied object contributes a collection at `+0x88`. For each examined pointer entry, the pointed object's collection at `+0x24` must be nonempty before the pointed object's 32-bit discriminator `+0x60` is dispatched. The specialized branches also require that pointed object's `+0x3C` collection to be nonempty. These offsets belong to the supplied/pointed objects; they do not identify equally numbered creature members. [Evidence: `binary/Cube.exe`, RVAs `0x1E9655–0x1E9677`, `0x1AF1A1–0x1AF1C3`, `0x1BC9AC–0x1BC9B8`, `0x1BCB62–0x1BCBCA`, `0x1BCF6A–0x1BCF7D`, `0x1BD146–0x1BD159`; `decompiled/client/functions/00477240.c`, RVA `0x077240`; `decompiled/client/assembly.txt`, same addresses.]

| Pointed-object discriminator | Descriptor byte `+0x30` | Stock call in `0x1AF170` | Reached stock routine |
| --- | --- | --- | --- |
| 4 | `0x82` | `0x1BD02F` | `0x1C8390` |
| 3 | `0x81` | `0x1BD20B` | `0x1CA0A0` |

Each branch allocates `0x10F0` bytes and invokes descriptor constructor `0x1ADB20`. A direct random call selects an element from the nonempty `+0x3C` collection by unsigned remainder modulo its element count; the selected value is passed to service `0x1A6A70` with descriptor position storage `+0x10`. Descriptor `+0x28` is then written as a 32-bit 3. A second direct random call supplies signed remainder modulo 2, then addition of 2, for the 32-bit kind at descriptor `+0x2C`. Descriptor `+0x34` receives the second supplied object's 32-bit `+0x24`, after which byte `+0x30` is assigned the table's value. Constructor-zeroed byte `+0x31` remains zero in these branches. These are the two explicit caller random draws; the wider constructor and position-service dependencies are not a closed upstream draw total. [Evidence: `binary/Cube.exe`, RVAs `0x1BCF83–0x1BD02F`, `0x1BD15F–0x1BD20B`, `0x1ADB30–0x1ADB44`; `decompiled/client/functions/005adb20.c`.]

Both stock calls supply the descriptor pointer followed by two 32-bit scalar stack arguments and the enclosing receiver in ECX. In each stock body only the descriptor stack argument is read; the two additional scalar arguments and incoming ECX do not determine stock. The stock routines do not read descriptor class, specialization, numeric role, kind or descriptor level to filter their candidates. The class byte is established by the caller before dispatch, rather than checked inside the stock routine. Other callers of these routines are not classified by this statement. [Evidence: `binary/Cube.exe`, RVAs `0x1BD006–0x1BD02F`, `0x1BD1E2–0x1BD20B`, `0x1C8390–0x1C9B1E`, `0x1CA0A0–0x1CADD2`.]

Descriptor inventory is the member at `+0xF6C`: the constructor explicitly passes that address to `0x03C020`, which initializes its outer collection pointers to empty, scalar `+0x0C` to zero, the assigned members of its embedded item at `+0x10` to their defaults, and its 32-bit counters `+0x128/+0x12C` to zero. After stock generation the caller finishes assembling a behavior object, stores it at descriptor `+0x109C`, and appends the retained descriptor pointer to the first supplied object's `+0x18` collection at `0x1BD527`. Generation therefore precedes this observed descriptor-pointer publication. The subsequent scheduling and actor creation are not closed here. [Evidence: `binary/Cube.exe`, RVAs `0x1ADBC1–0x1ADBFE`, `0x03C020–0x03C099`, `0x1BD034–0x1BD141`, `0x1BD210–0x1BD31D`, `0x1BD4F9–0x1BD52C`; `decompiled/client/functions/0043c020.c`, `0066add0.c`.]

## Band level and conversion boundary

Each stock body iterates a signed 32-bit band index from 0 through 29, ascending. At the beginning of every band, the index is converted to binary32 and divided by binary32 30, pattern `0x41F00000`. A second scalar input is formed by converting the same index, adding binary32 `0x3F7FFF58`—exactly `0.999989986419677734375`—and dividing by the same 30. Both inputs are separately evaluated by `0x0462F0`. That helper performs, in sequence, 1 minus the supplied input, 1 divided by that difference, subtraction of 1, multiplication by 20 and addition of 1. Those operations are individually binary32; 1 and 20 have patterns `0x3F800000` and `0x41A00000`. The near-one addend is preserved exactly as supplied by the image. [Evidence: `binary/Cube.exe`, RVAs `0x1C83D0–0x1C841F`, `0x1CA0E0–0x1CA12F`, `0x0462F0–0x046323`; constant RVAs `0x2FCD78`, `0x31DD4C`, `0x345DC0`, `0x2FCD74`; `decompiled/client/functions/004462f0.c`.]

The helper stores its binary32 result before loading the x87 return. Each caller immediately stores that return to binary32 and converts it to signed 32-bit integer with truncation toward zero. The lower input is evaluated and converted first, then the upper input. One random call follows. The signed 32-bit difference of upper minus lower, incremented by 1, becomes the signed remainder divisor for that draw; the remainder is added to the lower endpoint. A signed minimum of 1 is imposed. At band 0 the result is then forced to 1, after the helper evaluations, conversions and draw have already occurred. [Evidence: `binary/Cube.exe`, RVAs `0x1C83E5–0x1C8461`, `0x1CA0F5–0x1CA171`.]

The result's low 16 bits are stored once into the band's local item's level word and retained for all candidates in that band. There is no upper clamp or signed-word saturation. The high bands must therefore retain this explicit truncation boundary; a small positive signed item-level range is not proved. No actor or descriptor level is consulted for this arithmetic. Floating control settings, exceptional conversions/division, imported random range/distribution and nonordinary corrupted-state behavior remain outside the ordinary numeric conclusion. [Evidence: `binary/Cube.exe`, RVAs `0x1C8451–0x1C8496`, `0x1CA161–0x1CA19F`, and both full stock bodies.]

## Item construction shared by the two bounded bodies

Each band starts a fresh temporary candidate collection and a local semantic item. The item's auxiliary 32-bit member `+0x08`, extension count `+0x114` and all 256 bytes at `+0x14–+0x113` are set to zero. The initial word at item `+0x0D` is `0x0101`, establishing material byte 1 and byte `+0x0E` equal to 1. Later material-byte changes preserve that `+0x0E` value. The candidate's raw level word is the band value just described. Each candidate receives fresh tier and 32-bit random fields before its semantic copy is appended. [Evidence: `binary/Cube.exe`, RVAs `0x1C8433–0x1C852B`, `0x1CA143–0x1CA244`; `decompiled/client/functions/005c8390.c`, `005ca0a0.c`.]

| Item-relative storage | Width | Produced value or role |
| --- | --- | --- |
| `+0x00/+0x01` | One byte each | Category/subtype specified by the route below |
| `+0x04` | 32 bits | The complete result of a fresh random call; no modulo-100 reduction at this store |
| `+0x08` | 32 bits | Zero throughout these stock bodies |
| `+0x0C` | One byte | Independently generated candidate tier |
| `+0x0D` | One byte | Material selector specified below |
| `+0x0E` | One byte | 1, retained through candidate production and selection |
| `+0x10` | 16 bits | Raw low-word band level |
| `+0x14–+0x113` | 256 bytes | Explicitly zeroed; all 32 eight-byte entries participate in semantic copies |
| `+0x114` | 32 bits | Zero extension count |

For every candidate, its basic five-draw sequence is: one discarded random result; a draw tested for zero signed remainder modulo 100; another tested modulo 1000; another tested modulo 10000; and a final draw stored in full at `+0x04`. Tier begins at zero and is the number of the three successful remainder tests. The code applies a signed cap of 4 before storing the tier byte, although ordinary values from these tests are only 0 through 3. The discarded first draw does not provide a parity-based tier term. All three tests execute in their stated order without short-circuiting. Additional subtype/material/discard draws are specified separately below. [Evidence: `binary/Cube.exe`, RVAs `0x1C84B6–0x1C8502`, `0x1C8538–0x1C8587`, repeated candidate blocks through `0x1C997B`; `0x1CA1CF–0x1CA21B`, repeated candidate blocks through `0x1CAC2F`.]

Semantic copying preserves category/subtype bytes, 32-bit `+0x04/+0x08`, bytes `+0x0C/+0x0D/+0x0E`, the level word, all 32 eight-byte extension entries and the extension count. Item bytes `+0x02/+0x03`, `+0x0F` and `+0x12/+0x13` are not initialized by the scoped construction and are not copied by these item services. Their contents remain unspecified; this is not a complete raw copy of the `0x118`-byte item stride. The numeric selectors below do not establish display names, complete stat meanings or item eligibility in unrelated services. [Evidence: `binary/Cube.exe`, RVAs `0x00EE70–0x00EF10`, `0x1C1FA0–0x1C206C`; `decompiled/client/functions/0040ee70.c`, `0042c5e0.c`, `005c1fa0.c`.]

## Descriptor 0x82: body RVA 0x1C8390

Every band constructs 21 candidates, in the following append order. Subtype is explicitly initialized to zero before the first candidate and is not changed in this body. Each category-7 duplicate remains a separate candidate with its own five-draw sequence and stored random field. The rows represent unconditional construction, not class-admission branches. [Evidence: `binary/Cube.exe`, RVAs `0x1C8479–0x1C9A1A`; `decompiled/client/functions/005c8390.c`.]

| Candidate positions, zero-based | Material byte | Category order |
| --- | --- | --- |
| 0–3 | 1 | 7, 4, 5, 6 |
| 4–8 | 25 | 7, 7, 4, 5, 6 |
| 9–13 | 26 | 7, 7, 4, 5, 6 |
| 14–18 | 27 | 7, 7, 4, 5, 6 |
| 19 | Separately selected | 8 |
| 20 | Separately selected | 9 |

For candidate 19, and separately again for candidate 20, a fresh parity draw selects material 12 for even signed remainder modulo 2 and material 11 for odd. That material draw occurs before the candidate's discarded draw, three tier tests and full random-field draw. The category-8/9 candidates retain subtype zero, auxiliary zero, byte `+0x0E == 1`, the band's level and zero extensions. All 21 candidates are complete before any stock-count or candidate-selection draw. [Evidence: `binary/Cube.exe`, RVAs `0x1C97DA–0x1C9A20`.]

## Descriptor 0x81: body RVA 0x1CA0A0

Every band constructs eleven category-3 candidates, using the ordered subtype/material assignments below. All rows are unconditional. Each listed candidate has the same basic five-draw sequence for its own tier and full random field. [Evidence: `binary/Cube.exe`, RVAs `0x1CA1AF–0x1CACCE`; `decompiled/client/functions/005ca0a0.c`.]

| Candidate position, zero-based | Subtype | Material byte | Additional draws before its basic sequence |
| --- | --- | --- | --- |
| 0 | Signed remainder modulo 3 | 1 | One subtype draw |
| 1 | 15 plus signed remainder modulo 3 | 1 | One new subtype draw |
| 2 | 13 | 1 | None |
| 3 | 3 | 1 | None |
| 4 | 5 | 1 | None |
| 5 | 4 | 1 | None |
| 6 | 6 | 2 | None |
| 7 | 8 | 2 | Five discarded draws after candidate 6 is appended and before subtype 8 is written |
| 8 | 10 | 2 | None |
| 9 | 11 | 2 | None |
| 10 | 12 | Separately selected | One parity draw: material 12 for even, 11 for odd |

In particular, the five consecutive calls between candidates 6 and 7 are retained draws with no supplied result carried into the subtype-8 item. They are additional to subtype 8's own discarded draw and other four basic draws. No candidate or stock entry is appended for that five-call interval. Candidate 10's parity draw precedes its five-draw basic sequence. The full eleven-item pool is constructed before drawing the stock count. [Evidence: `binary/Cube.exe`, RVAs `0x1CA77C–0x1CA89F`, `0x1CABB1–0x1CACD4`.]

## Selection, removal and final inventory

After constructing a band's complete pool, each body draws a stock count using signed remainder modulo 3 plus 1. For ordinary nonnegative random results this requests 1, 2 or 3 entries. While the requested number has not been reached and the pool is nonempty, a fresh random call is divided unsigned by the current pool count. The remainder indexes a candidate in the current append order. That item's address is supplied to inventory-add `0x06EBE0`, with receiver descriptor `+0xF6C` and group argument -1. Selection is therefore after candidate construction, and successful inventory insertion precedes pool removal. [Evidence: `binary/Cube.exe`, RVAs `0x1C9A20–0x1C9A7E`, `0x1CACD4–0x1CAD32`.]

Removal uses `0x1C1FA0` to copy every candidate following the selected item one position toward the front in ascending order. These are semantic item assignments. The caller then reduces the pool end by exactly `0x118` bytes. The next selection draw uses the reduced count and that stable remaining order. Choosing the last element requires no following-item copies. A previously selected candidate record cannot be selected a second time within that band's pool, although distinct candidates can have equal semantic values. After the requested selections, the remaining temporary pool allocation is released, the band index increments, and a fresh pool is built for the next band. [Evidence: `binary/Cube.exe`, RVAs `0x1C9A83–0x1C9AFD`, `0x1CAD37–0x1CADB1`, `0x1C1FA0–0x1C206C`; `decompiled/client/functions/005c1fa0.c`.]

Group argument -1 reaches category-based group selection `0x07FA30`. All categories supplied by these two bodies—3 through 9—select group 0. Inventory-add copies the semantic item, gives it a quantity contribution of 1, and retains its raw level word: none of these categories enters its level-as-quantity normalization or its category-12/category-13 counter routes. The supplied categories also fail stackability predicate `0x07F9F0`, so their equality is not used to merge stock, including semantically equal candidates. [Evidence: `binary/Cube.exe`, RVAs `0x06EC0A–0x06ED12`, `0x06ED8A–0x06EDAF`, `0x07FA30–0x07FA6D`, dispatch tables at `0x07FA70/0x07FA80`, `0x07F9F0–0x07FA0F`; `decompiled/client/functions/0046ebe0.c`, `0047fa30.c`, `0047f9f0.c`.]

Inventory group descriptors have 12-byte stride; entries have `0x11C`-byte stride, with a 32-bit quantity followed by the semantic item. On an existing inventory, insertion scans entries in ascending order, remembers the first quantity exactly equal to zero and reuses that slot when no stackable equality applies; otherwise it appends an entry. In these fresh-descriptor routes there are initially no entries and every supplied contribution is 1, so the stock entries append in band order and, within a band, selection order. Each ordinary completed descriptor receives 30 through 90 entries, all in group 0 and all quantity 1. Their categories remain 3–9; these bodies do not recategorize the selected items as category 2 or replace auxiliary `+0x08`. [Evidence: `binary/Cube.exe`, RVAs `0x06EC26–0x06EC51`, `0x06ED12–0x06EE95`, `0x086700–0x086773`; `decompiled/client/functions/00487380.c`, `00427a80.c`, `00486700.c`; the two full stock bodies.]

Neither stock routine clears the descriptor's existing inventory or replaces an entire stock group. Its cleared/reset collection is the temporary candidate pool for each band. Therefore a hypothetical later call on the same valid inventory would add entries or reuse zero-quantity slots rather than automatically discard old stock. This statement does not prove that such repeated calls occur. In the observed branches the descriptor is newly constructed, stock is generated once before pointer publication, and later refresh or replacement policy is unclosed. Pool-item addresses are temporary; inventory receives semantic copies before those addresses are removed or their allocation is released. [Evidence: `binary/Cube.exe`, RVAs `0x1BCF83–0x1BD02F`, `0x1BD15F–0x1BD20B`, `0x1C8390–0x1C9B1E`, `0x1CA0A0–0x1CADD2`, `0x06EBE0–0x06EEAA`.]

## Random-call accounting and limits

The calls use the original named import `MSVCR110.dll!rand`, at IAT RVA `0x2FC468`. For the `0x82` body, a band consumes one level draw, then the 21 candidates' 105 basic draws plus two material draws, then one count draw, then one draw per selected stock entry. If the requested ordinary count is N, this is 109 plus N draws per band. For the `0x81` body, a band consumes one level draw, then eleven candidates' 55 basic draws, two subtype draws, five extra discarded draws and one material draw, then one count draw and N selection draws: 65 plus N draws per band. The order is the construction and selection order described above. [Evidence: `binary/Cube.exe`, full bodies `0x1C8390–0x1C9B1E`, `0x1CA0A0–0x1CADD2`, and import directory/IAT RVA `0x2FC468`.]

Thus an ordinary successfully completed 30-band stock call has 3300 through 3360 direct random calls for descriptor `0x82`, or 1980 through 2040 for descriptor `0x81`. These totals include draws spent on candidates that never become stock and band-zero level draws whose result is overridden. They exclude the two explicit upstream descriptor draws and any unclosed upstream service effects. No reseed, private seed, random-state save/restore or random retry appears in either stock body. These are draw-order and call-count findings; the imported generator's internal state management, range and distribution have not been audited. [Evidence: `binary/Cube.exe`, the same bounded bodies and their caller intervals cited above.]

Reached nested work is bounded to the scalar level helper, candidate capacity/copy services, semantic removal, inventory admission and imported allocation/disposal/security services. Capacity growth `0x0C6940` can reach `0x0C6840` and the ordered semantic-copy loop `0x0C67B0`; these support pool order under ordinary successful allocation. No complete exception, allocator, concurrent-access, protection or rollback guarantee follows. The original supplied-object producers, broader role/class creation rules, position-service semantics, descriptor/actor ownership and synchronization, stock refresh cadence, UI/payment consumers, item display/stat interpretation and other generation branches remain outside this contract. Server behavior is not inferred from either client route. [Evidence: `decompiled/client/functions/004c6940.c`, `004c6840.c`, `004c67b0.c`, `0046ebe0.c`, `005c1fa0.c`, `005c8390.c`, `005ca0a0.c`; `binary/Cube.exe`, corresponding RVAs and inspected caller boundaries.]
