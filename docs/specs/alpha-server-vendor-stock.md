# Original server descriptor stock routes

Status: FINAL / STABLE / HELD.

The supported original is the 32-bit x86 PE image `binary/Server.exe`, preferred base `0x00400000`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. All addresses below are server RVAs. Relative evidence paths begin at `Documents/disassembled alpha cube world`. This contract uses the independently inspected server image, its assembly listing and the identified server decompilations. Client offsets, constants and behavior were not transferred as evidence; no earlier contract, reconstruction, implementation or runtime material was used.

The closed ordinary stock bodies are `0x0FD920`, `0x0FC180` and `0x0FDE90`, reached after assigning descriptor class bytes `0x80`, `0x82` and `0x81` respectively. Reached item selectors `0x128BF0/0x12C4E0`, level helper `0x011090`, semantic copying/removal and inventory insertion are covered to the boundaries below. This is not a complete server generation or vendor-lifecycle contract. Native instructions take precedence over misleading decompiled local types or simplified container expressions. [Evidence: `binary/Server.exe`, the named function bodies; `decompiled/server/functions/004fd920.c`, `004fc180.c`, `004fde90.c`, `00528bf0.c`, `0052c4e0.c`, `00411090.c`; `decompiled/server/assembly.txt`, corresponding function intervals.]

## Original callers, admissions and storage

The observed outer call is at `0x11D452` inside routine `0x118630`: it calls `0x0E28E0` when the supplied second object's 32-bit `+0x18` equals 1 or 5, passing a first object, that second object, and the enclosing receiver in ECX. Inside `0x0E28E0`, the three branches audited here are under the second object's `+0x18 == 1` branch. The first object supplies a pointer-valued collection at `+0x88`. For an examined entry, the pointed object's collection at `+0x24` must be nonempty before dispatching its 32-bit `+0x60` discriminator. Each audited discriminator branch additionally requires the pointed object's `+0x3C` collection to be nonempty. Empty tests compare the first two collection pointer words. The supplied and pointed objects' wider identities and producers are not established by those offsets alone. [Evidence: `binary/Server.exe`, RVAs `0x11D435–0x11D457`, `0x0E2911–0x0E2939`, `0x0F011C–0x0F0128`, `0x0F02D2–0x0F033D`, `0x0F04FE–0x0F0511`, `0x0F06DA–0x0F06ED`, `0x0F08B6–0x0F08C9`, `0x0E28D0–0x0E28DA`; `decompiled/server/assembly.txt`.]

| Pointed-object discriminator | Descriptor class byte `+0x30` | Server stock body | Call site |
| --- | --- | --- | --- |
| 2 | `0x80` | `0x0FD920` | `0x0F05C3` |
| 4 | `0x82` | `0x0FC180` | `0x0F079F` |
| 3 | `0x81` | `0x0FDE90` | `0x0F097B` |

Each branch allocates `0x10F0` bytes and constructs a new descriptor with `0x0E0F40`. One explicit random draw selects an element of the pointed object's nonempty `+0x3C` collection using unsigned remainder modulo its count. The selected value is passed to `0x0D99D0` for descriptor `+0x10` position storage. Next, descriptor `+0x28` receives a 32-bit 3, and a second explicit random draw supplies signed remainder modulo 2 plus 2 for the 32-bit kind at `+0x2C`. Descriptor level `+0x34` receives the second supplied object's 32-bit `+0x24`; byte `+0x30` then receives the table's class. Constructor-initialized byte `+0x31` stays zero. The constructor/position services' wider effects are unclosed, so the two explicit caller draws are not an upstream total. [Evidence: `binary/Server.exe`, RVAs `0x0F0517–0x0F05C3`, `0x0F06F3–0x0F079F`, `0x0F08CF–0x0F097B`; `decompiled/server/functions/004e0f40.c`, RVA `0x0E0F40`.]

The stock calls receive the descriptor pointer as their first stack argument, followed by two 32-bit scalar stack arguments. Those extra scalar arguments are not read by any of the three stock bodies. The `0x80` body saves incoming ECX and forwards it to its reached item selectors, which do not inspect that receiver. The two specialized bodies do not use incoming ECX as a stock input. All three obtain their inventory receiver by adding `0xF6C` to the supplied descriptor. They do not read descriptor class, specialization, numeric role, kind or descriptor level to determine stock contents. The byte assignment and stock dispatch are concrete caller operations, not a class-byte test inside the bodies. [Evidence: `binary/Server.exe`, RVAs `0x0F059A–0x0F05C3`, `0x0F0776–0x0F079F`, `0x0F0952–0x0F097B`, `0x0FD920–0x0FDD58`, `0x0FC180–0x0FD90E`, `0x0FDE90–0x0FEBC2`, `0x128BF0–0x1290C5`, `0x12C4E0–0x12C811`.]

Descriptor constructor `0x0E0F40` explicitly initializes inventory `+0xF6C` by calling `0x006EF0`. Its three outer collection pointers and scalar `+0x0C` start zero; the assigned semantic members of its embedded item at `+0x10` start empty with level word 1, and counters `+0x128/+0x12C` start zero. No all-zero allocation or padding guarantee follows. After stock generation the enclosing branch finishes assembling a behavior object, writes descriptor `+0x109C`, and publishes the retained descriptor pointer through the first supplied object's `+0x18` collection at `0x0F0C97`. This is the observed publication boundary; subsequent actor creation, refresh and retention remain unclosed. [Evidence: `binary/Server.exe`, RVAs `0x0E0FE1–0x0E1019`, `0x006EF0–0x006F69`, `0x0F0C69–0x0F0C9C`; `decompiled/server/functions/00406ef0.c`; `decompiled/server/assembly.txt`, corresponding caller branches.]

## Band arithmetic in the server image

Each body iterates a signed 32-bit band index from 0 through 29 in ascending order. The first input to `0x011090` is the index converted to binary32 and divided by binary32 30. The second input first adds the server's binary32 near-one constant to that converted index, then divides by 30. Independently decoded server constants are: 30 at RVA `0x158754`, bits `0x41F00000`; near-one at `0x1739D8`, bits `0x3F7FFF58`, exactly `0.999989986419677734375`; 1 at `0x1586DC`, bits `0x3F800000`; and 20 at `0x15874C`, bits `0x41A00000`. [Evidence: `binary/Server.exe`, constant bytes and RVAs `0x0FD953–0x0FD9A2`, `0x0FC1C0–0x0FC20F`, `0x0FDED0–0x0FDF1F`.]

Helper `0x011090` subtracts the supplied input from binary32 1, divides 1 by that difference, subtracts 1, multiplies by 20 and adds 1, with a distinct scalar binary32 operation at each step. It stores the result as binary32 before loading the x87 return. The callers immediately store that return as binary32 again, then truncate it toward zero into a signed 32-bit integer. The lower input is evaluated and converted before the upper input. One random call follows both conversions. Signed 32-bit upper-minus-lower, incremented by 1, supplies the signed remainder divisor for that draw. The remainder is added to the lower integer and clamped to a signed minimum of 1. Band 0 then forces the result to 1 after all these evaluations and the draw. [Evidence: `binary/Server.exe`, RVAs `0x011090–0x0110C3`, `0x0FD968–0x0FD9E4`, `0x0FC1D5–0x0FC251`, `0x0FDEE5–0x0FDF61`; `decompiled/server/functions/00411090.c`.]

There is no maximum clamp or saturation to a positive signed item-level range. The `0x80` body retains the band result in a 32-bit local, passes it to its selected item helper, and eventually stores its low word for the final category-1 item. Each nested item helper reads only the low 16 bits for its item level. The specialized bodies store the low word once for the band and retain it for all candidates. Floating control state, invalid conversions, exceptional division, imported random range/distribution and malformed storage are outside the ordinary finite-input arithmetic closure. [Evidence: `binary/Server.exe`, RVAs `0x0FD9CA`, `0x0FDC49–0x0FDC69`, `0x0FDD05–0x0FDD0E`, `0x128C78–0x128C85`, `0x12C55B–0x12C568`, `0x0FC286`, `0x0FDF8F`.]

## Item fields and copy boundaries

Items occupy `0x118`-byte strides. The table gives the assigned semantic fields for this audit. Numeric category, subtype and material identities are preserved without assigning display names or a complete stat meaning.

| Item-relative field | Width | Established use |
| --- | --- | --- |
| `+0x00/+0x01` | One byte each | Category and subtype selectors |
| `+0x04` | 32 bits | Fixed zero, a modulo-100 random field, or a full random result as distinguished below |
| `+0x08` | 32 bits | Zero, except the original category saved for `0x80` category-2 stock |
| `+0x0C` | One byte | Tier |
| `+0x0D` | One byte | Material selector |
| `+0x0E` | One byte | Zero in the `0x80` constructions; 1 in specialized candidates |
| `+0x10` | 16 bits | Raw item-level word |
| `+0x14–+0x113` | 256 bytes | Explicitly zeroed extension area |
| `+0x114` | 32 bits | Zero extension count |

Item construction/copy `0x013710`, assignment `0x002A70` and removal-copy helper `0x0F59F0` transfer the listed scalar fields and all 32 complete eight-byte entries in the extension area. Item bytes `+0x02/+0x03`, `+0x0F`, `+0x12/+0x13` are neither initialized by the scoped semantic constructors nor copied by those services. Their values are unspecified. A zero extension count does not suppress copying the full cleared extension area. These operations are semantic copies, not complete raw copies of the item stride. [Evidence: `binary/Server.exe`, RVAs `0x013710–0x0137B0`, `0x0078F0–0x0079B2`; `decompiled/server/functions/00413710.c`, `00402a70.c`, `004f59f0.c`; the three stock bodies and reached item selectors.]

## Descriptor class 0x80: fixed and generated stock

Body `0x0FD920` always inserts into explicit inventory group 0. At band 0 it creates a local item with `+0x04/+0x08`, tier, byte `+0x0E`, extension area and extension count zero and level word 1. It then performs these operations in order, reusing the local item and retaining all unmentioned semantic fields. [Evidence: `binary/Server.exe`, RVAs `0x0FD9D6–0x0FDB7D`; `decompiled/server/functions/004fd920.c`.]

| Order | Category/subtype | Tier | Material | Admission |
| --- | --- | --- | --- | --- |
| First | 11 / 12 | 0 | 24 | Always |
| Second | 24 / 0 | 0 | 1 | Always |
| Four optional checks, ascending tier | 24 / 0 | 1, 2, 3, 4 respectively | 1 | One draw each; signed remainder modulo 10, 20, 30, 40 respectively must equal zero |
| Next | 23 / 0 | 0 | 2 | Always |
| Next | 23 / 1 | 0 | 2 | Always |
| Last fixed item | 20 / selected subtype | 0 | 0 | Always; one subtype draw |

The four optional draws all occur, without retries. Writing word `0x0200` at item `+0x0C` resets the tier before category 23. For the last row, signed remainder modulo 6 selects subtype `0x22`, `0x23`, `0x13`, `0x1A`, `0x1E`, `0x57` for remainders 0–5 respectively. The fixed items retain level 1, zero random/auxiliary fields, byte `+0x0E` zero and zero extensions. [Evidence: `binary/Server.exe`, RVAs `0x0FDA75–0x0FDB7D`, including the jump table at `0x0FDD5C`.]

After any band-zero fixed stock, one draw's signed remainder modulo 3 supplies the generated category-2 count for that band: ordinarily 0, 1 or 2. For each, a new parity draw gives the initial tier 0 or 1. Three further draws, always in order, add 1 for zero signed remainder modulo 100, then 1000, then 10000. A signed maximum of 4 is imposed. A fifth draw selects item helper `0x128BF0` for even parity or `0x12C4E0` for odd. The call supplies an output-item address, the band result, tier and selector -1. Both helpers ignore the forwarded ECX receiver; selector -1 admits all their numbered branches. [Evidence: `binary/Server.exe`, RVAs `0x0FDB82–0x0FDC69`, `0x128BF0–0x1290C5`, `0x12C4E0–0x12C811`.]

Before the family draw the stock body's local item is semantically reset, including level 1 and zero extensions. The selected helper's output is assigned into it. The helper-produced category is zero-extended into 32-bit `+0x08`, then the category byte alone becomes 2. Subtype, random field, supplied tier, material, byte `+0x0E == 0`, raw level and zero extensions retain the helper output's values. The item is inserted immediately before generating the next one. [Evidence: `binary/Server.exe`, RVAs `0x0FDBDF–0x0FDCA9`; `decompiled/server/functions/00402a70.c`.]

After the generated items, one new parity draw always occurs. Even parity adds nothing; odd constructs category 1/subtype 7 with the band's low-word level and zero `+0x04/+0x08`, tier, material, byte `+0x0E`, extension area and extension count. It is inserted into group 0 before the band advances. There is no additional role/class/descriptor-level gate. [Evidence: `binary/Server.exe`, RVAs `0x0FDCB7–0x0FDD42`.]

## Nested item selectors reached by class 0x80

Selector `0x128BF0` makes a temporary candidate pool. The supplied tier's low byte and supplied level's low word are retained for every candidate; subtype is zero, auxiliary `+0x08`, byte `+0x0E`, extension area and extension count are zero. Each candidate gets a fresh signed remainder modulo 100 at 32-bit `+0x04` immediately before append. A numbered branch is admitted when its selector argument equals that number or is signed-negative. Thus the actual -1 call builds its first nineteen candidates in this sequence. [Evidence: `binary/Server.exe`, RVAs `0x128C1E–0x128FDA`; `decompiled/server/functions/00528bf0.c`, `00528530.c`.]

| Branch selector | Material | Categories in append order |
| --- | --- | --- |
| 1 | 1 | 7, 4, 5, 6 |
| 3 | 25 | 7, 7, 4, 5, 6 |
| 2 | 26 | 7, 7, 4, 5, 6 |
| 4 | 27 | 7, 7, 4, 5, 6 |

Category 8 and then category 9 always follow. Each uses a fresh parity draw selecting material 11 for even and 12 for odd, then a fresh modulo-100 draw for `+0x04`, then append. One final draw is divided unsigned by the candidate count, 21 for selector -1; its remainder chooses a zero-based candidate in append order. The chosen semantic item is copied into the supplied output; the pool allocation is released and the output pointer is returned. Duplicated category-7 records remain distinct. This reached call consumes 24 random draws: 21 candidate-field draws, two material draws and one selection draw. [Evidence: `binary/Server.exe`, RVAs `0x128FE6–0x1290C5`; `decompiled/server/functions/00528bf0.c`.]

Selector `0x12C4E0` similarly retains the supplied tier/low-word level and zero auxiliary, byte `+0x0E` and extensions. Its category is 3. Under selector -1 it admits branches in order 1, 4, 2, 3, building eleven candidates as below. Each append gets a separate modulo-100 draw for `+0x04`. [Evidence: `binary/Server.exe`, RVAs `0x12C50E–0x12C7A1`; `decompiled/server/functions/0052c4e0.c`.]

| Branch selector | Ordered subtypes | Material and additional random work |
| --- | --- | --- |
| 1 | Remainder modulo 3; 15 plus another remainder modulo 3; 13 | Material 1; each of the first two subtype draws precedes its field draw |
| 4 | 3, 5, 4 | Material 1 retained |
| 2 | 6, 8 | Material 2; one discarded draw after subtype 6 is appended and before subtype 8 is assigned |
| 3 | 10, 11, 12 | Material 2 for the first two; for subtype 12, a parity draw selects material 12 for even, 11 for odd before its field draw |

A final draw is divided unsigned by the eleven-candidate count. The remainder chooses the output in append order; the chosen semantic item is copied, the temporary allocation released and output pointer returned. This reached call consumes 16 draws: eleven candidate-field draws, two subtype draws, one discarded draw, one material draw and one selection draw. Its discarded interval contains one draw, not the specialized body's five-draw interval described below. [Evidence: `binary/Server.exe`, RVAs `0x12C596–0x12C811`; `decompiled/server/functions/0052c4e0.c`.]

## Specialized stock bodies: classes 0x82 and 0x81

The specialized bodies construct every candidate anew for every band, before drawing how many to insert. The band-local auxiliary field, extension count and area are zero; word `0x0101` at item `+0x0D` establishes material 1 and byte `+0x0E == 1`. Subsequent material changes leave that byte equal to 1. Each candidate's basic sequence is five calls: one discarded result, a modulo-100 zero test, a modulo-1000 zero test, a modulo-10000 zero test, and a final full random result stored at `+0x04`. Tier starts at zero and counts the three successes, then a signed cap of 4 is applied before storing its byte. The ordinary reachable tier is 0–3. The discarded first call supplies no initial parity term. Each test executes even when another succeeded. [Evidence: `binary/Server.exe`, RVAs `0x0FC223–0x0FC31B`, repeated candidate blocks through `0x0FD80A`; `0x0FDF33–0x0FE034`, repeated blocks through `0x0FEABE`; `decompiled/server/functions/004fc180.c`, `004fde90.c`.]

Class `0x82` body `0x0FC180` builds 21 candidates unconditionally, subtype zero throughout. Its first nineteen categories/materials are independently established in the server body as material 1 with categories 7, 4, 5, 6; material 25 with 7, 7, 4, 5, 6; material 26 with 7, 7, 4, 5, 6; then material 27 with 7, 7, 4, 5, 6. Category 8 then category 9 complete the pool. Each of those last two first draws its material, 12 for even parity or 11 for odd, then executes its own five-call basic sequence. The duplicate category-7 entries have independent tiers/random fields and remain separate candidates. These are unconditional constructions, not class-selector branch tests. [Evidence: `binary/Server.exe`, RVAs `0x0FC269–0x0FD80A`; `decompiled/server/functions/004fc180.c`.]

Class `0x81` body `0x0FDE90` unconditionally builds eleven category-3 candidates. Their ordered subtype/material and extra draws are the following; every row additionally has its own five-call basic sequence. [Evidence: `binary/Server.exe`, RVAs `0x0FDF9F–0x0FEABE`; `decompiled/server/functions/004fde90.c`.]

| Zero-based position | Subtype | Material | Extra draws before its basic sequence |
| --- | --- | --- | --- |
| 0 | Signed remainder modulo 3 | 1 | One subtype draw |
| 1 | 15 plus signed remainder modulo 3 | 1 | Another subtype draw |
| 2 | 13 | 1 | None |
| 3 | 3 | 1 | None |
| 4 | 5 | 1 | None |
| 5 | 4 | 1 | None |
| 6 | 6 | 2 | None |
| 7 | 8 | 2 | Five consecutive discarded draws after candidate 6 is appended and before subtype 8 is assigned |
| 8 | 10 | 2 | None |
| 9 | 11 | 2 | None |
| 10 | 12 | Separately selected | One parity draw, material 12 for even and 11 for odd |

The five discarded draws are additional to candidate 7's own discarded basic draw; no candidate append occurs within that five-call interval. The two specialized bodies store complete random results at `+0x04`, not modulo-100 values. Their candidates retain categories 3–9, auxiliary zero, byte `+0x0E == 1`, the raw band-level word and zero extensions when selected for stock. No category-2 conversion occurs in either specialized body. [Evidence: `binary/Server.exe`, RVAs `0x0FE67C–0x0FE68F`, `0x0FE9A1–0x0FEA1F`, `0x0FD856–0x0FD86E`, `0x0FEB0A–0x0FEB22`; both full specialized bodies.]

After a complete specialized pool is built, a count draw's signed remainder modulo 3 plus 1 requests ordinarily 1–3 entries. For each requested entry while the pool is nonempty, a fresh draw is divided unsigned by the current candidate count; its remainder chooses in current pool order. Inventory-add receives descriptor `+0xF6C`, the chosen item address and group argument -1. Insertion precedes removal. Helper `0x0F59F0` then semantically copies every following candidate one slot toward the front, ascending, and the caller decreases the pool end by `0x118`. Later choices use the reduced count and stable remaining order. The selected candidate cannot recur within the same band; a distinct equal candidate can still be chosen. The leftover temporary allocation is released before advancing to the next band. [Evidence: `binary/Server.exe`, RVAs `0x0FD810–0x0FD8ED`, `0x0FEAC4–0x0FEBA1`; `decompiled/server/functions/004f59f0.c`.]

## Group, quantity, equality and retention

Inventory-add is server RVA `0x027000`, with inventory object in ECX, supplied item pointer and 32-bit group argument on the stack. Group -1 calls category selector `0x0282F0`. Categories 3–9 all map to group 0 in its server jump tables, so specialized stock reaches group 0. The `0x80` body supplies group 0 explicitly and bypasses that selector. Group descriptors have 12-byte stride; entries have `0x11C`-byte stride, comprising a 32-bit quantity followed by a semantic item. [Evidence: `binary/Server.exe`, RVAs `0x027000–0x027071`, `0x027132–0x02718E`, `0x0282F0–0x02832D`, jump tables `0x028330/0x028340`; `decompiled/server/functions/00427000.c`, `004282f0.c`.]

Every scoped insertion contributes quantity 1. For the reached fixed categories 11/subtype 12, 24, 23 and 20, insertion obtains the signed item level and changes the semantic copy's level to 1; each supplied fixed level is already 1. Categories 1, 2 and 3–9 retain contribution 1 and the supplied raw level. No produced item takes the category-12 or category-13 counter routes. Insertion copies the item rather than changing the caller's local semantic item through those normalization writes. [Evidence: `binary/Server.exe`, RVAs `0x02706A–0x027132`; `decompiled/server/functions/00427000.c`.]

Scanning is in ascending entry order, retaining the first quantity exactly equal to zero as a possible reuse slot. Stackability `0x0282D0` is tested on the supplied item and admits categories 1, 10, 12, 13, 11 and 21. Thus only category 1 and the fixed category-11 item are stackable among the scoped stock. The first eligible equality match receives 32-bit quantity addition and returns, without requiring its old quantity to be positive. Equality `0x0078F0`, for these zero-extension-count items, compares category, subtype, 32-bit `+0x04`, byte `+0x0E`, 32-bit `+0x08`, tier, raw level word, material and extension count; it does not compare padding or extension entries at count zero. Nonzero/malformed extension-count behavior is not needed to close generated stock and remains outside this statement. [Evidence: `binary/Server.exe`, RVAs `0x0271A0–0x02723E`, `0x0282D0–0x0282EF`, `0x0078F0–0x0079B2`; `decompiled/server/functions/004078f0.c`.]

Without a match, insertion semantically replaces the first zero-quantity slot if one was recorded, setting its quantity to the contribution; otherwise it appends quantity and semantic item. The freshly constructed inventories have no old slots and every contribution is positive 1. Specialized stock therefore appends 30–90 separate entries in band order and within-band selection order, all quantity 1. Class `0x80` category-2 and fixed nonstackable items also remain separate; repeated equal category-1 items can merge into their first occurrence. No inventory-wide clear, stock-group replacement or refresh occurs inside these three stock bodies. A hypothetical later call on existing valid storage would insert/reuse slots rather than automatically replace old stock; the occurrence and schedule of such calls are not established. [Evidence: `binary/Server.exe`, RVAs `0x027241–0x0272CA`; `decompiled/server/functions/00427000.c`, the three stock bodies and descriptor constructor.]

## Random order and unresolved services

The independently verified import is named `MSVCR110.dll!rand`, IAT RVA `0x1582F4`. No dedicated stock seed, reseed, random-state save/restore or retry appears in the three bodies or the two scoped item selectors. For class `0x80`, each band consumes its level draw, then at band 0 the four optional checks and subtype draw, then its category-2 count draw, each admitted category-2 item's complete draws, and its final category-1 parity draw. A generated category-2 item consumes four tier draws plus one family draw, followed by all 24 or 16 selected-helper draws: 29 or 21 total. Thus the body has three direct draws per band, five additional direct draws at band 0, and those per-generated-item totals. No uniform probability claim follows from modulo selection. [Evidence: `binary/Server.exe`, import directory/IAT `0x1582F4`, RVAs `0x0FD920–0x0FDD58`, `0x128BF0–0x1290C5`, `0x12C4E0–0x12C811`.]

For each class-`0x82` band the order is level draw; 21 basic five-call sequences plus two material draws in candidate order; count draw; then one selection draw per inserted entry. With an ordinary requested count N, this is 109 plus N draws per band and 3300–3360 over thirty completed bands. For class `0x81` it is level draw; eleven basic sequences plus two subtype draws, five extra discarded draws and one material draw in their established positions; count draw; then N selection draws. This is 65 plus N per band and 1980–2040 per completed call. These counts include candidates never inserted and band-zero level draws whose value is overridden. They exclude the caller's two explicit draws and unclosed upstream services. [Evidence: `binary/Server.exe`, RVAs `0x0FC180–0x0FD90E`, `0x0FDE90–0x0FEBC2`.]

The ordinary conclusions require successful allocation, valid source collections and ordinary completion of copy/admission services. Candidate growth `0x013940`, append `0x128530`, inventory group growth `0x01F770`, entry append `0x028630` and imported allocation/disposal/security machinery are service boundaries; their complete failure, exception, synchronization and lifetime behavior is not closed. The candidate values and direct random order above are closed without claiming those broader guarantees. The enclosing receiver's wider role, upstream descriptor/position generation, actor ownership and lifetime, later stock refresh, transport/UI/payment consumers, full item-stat semantics and unrelated generation branches remain unresolved. This server-only contract neither infers client equivalence nor establishes a complete server vendor system. [Evidence: `decompiled/server/functions/00528530.c`, `00427000.c`, `004fc180.c`, `004fd920.c`, `004fde90.c`, `00528bf0.c`, `0052c4e0.c`; `binary/Server.exe`, scoped bodies and caller intervals cited above.]
