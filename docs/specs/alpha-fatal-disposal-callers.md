Status: FINAL / STABLE / HELD. Full author readback completed.

# Hit application: fatal disposal, fatal loot and upstream callers

This independently authored, code-free contract resolves three items that `alpha-fatal-branch-order.md` lists as unresolved: the virtual disposal calls of its step F9, the fatal-loot helper called at its step F5, and the callers of the hit-application routine that contains the fatal branch. Step names F1 to F12 refer to that contract, which was used to locate routines; every fact below was re-established from the original images. The Infinite-Cubes reconstruction was consulted only as a lead for where to look; nothing here rests on it.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`; every address below is an RVA, written client / server. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method

Routines were read in `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. Where the client listing omits instruction runs, the bytes were read from the client image. Client and server were compared instruction by instruction after replacing absolute addresses, and every differing absolute address (callee, constant, import slot, jump table, static object) was resolved in its own image. Constants and tables were read from image bytes; zero-filled static data was traced to its start-up initialiser. Import slots were resolved by walking each image's import directory. Run-time type information (RTTI) strings were read from image bytes. Statements that a routine makes no random draw, writes no health, or does not reach a given routine rest on the call graph `decompiled/*/calls.csv`, followed transitively; any such search also listed every unresolved indirect call. Health-write searches matched stores to offset `+0x16C` with a plain base register. Other addressing forms were not searched.

Random source: the import slot at `0x2FC468` / `0x1582F4`, which resolves to `MSVCR110.dll!rand` in each image. A **draw** is one call through it.

## 2. Routines

| Role | Client | Server |
| --- | --- | --- |
| Hit-application routine (contains the fatal branch) | `0x195A60` | `0x0CEA80` |
| Attack routine (four hit calls) | `0x196D30` | `0x0CFD50` |
| World update routine (four hit calls) | `0x20C510` (and its range `0x2215E0–0x222B96`) | `0x1322D0` (and its range `0x1473A0–0x148956`) |
| Fatal-loot helper | `0x19B180` | `0x0D2AE0` |
| Ground-drop routine | `0x19AEB0` | `0x0D2810` |
| Rarity roll | `0x1F8530` | `0x12BF40` |
| Item generator P | `0x1F8AD0` | `0x12C4E0` |
| Item generator Q | `0x1F51E0` | `0x128BF0` |
| Level variation | `0x0C7E80` | `0x014470` |
| Inverse level curve | `0x0462F0` | `0x011090` |
| Level-to-fraction curve | `0x03CA60` | `0x007D60` |
| Pet-food generator | `0x1F79E0` | `0x12B3F0` |
| Formula generator | `0x1F6D50` | `0x12A760` |
| Power scale | `0x045F60` | `0x010F00` |
| Item clear | `0x02F3E0` | `0x006FD0` |
| Offset-to-fixed conversion | `0x02C460` | `0x002510` |
| Fixed vector addition | `0x02C800` | `0x002CB0` |
| Binary32-to-signed-64 truncation | `0x28D946` | `0x14A946` |
| Drop-record initialiser | `0x04A970` | `0x01D8D0` |
| Item copy | `0x02C5E0` | `0x002A70` |
| Resident (zone) lookup | `0x034D10` | `0x006290` |
| Column block accessor | `0x02F730` | `0x005F20` |
| Zone item append | `0x04D590` | `0x01F5B0` |
| Zone-set insertion | `0x18E7D0` | `0x0DA680` |
| Spawn record constructor | `0x1ADB20` | `0x0E0F40` |
| Spawn record destructor body | `0x1ADCA0` | `0x0E10C0` |
| Spawn record deleting destructor (virtual slot 0) | `0x1ADDB0` | `0x0E11D0` |
| Element-buffer release | `0x239CF0` | `0x006310` |
| Zone constructor | `0x2240F0` | `0x148B60` |
| Zone generation (sets base count) | `0x1E4850` | `0x118630` |
| Maximum-health routine | `0x044DB0` | `0x00FDA0` |
| Slot health bonus | `0x0C70B0` | `0x013CE0` |
| Damageable predicate | `0x03E310` | `0x009620` |
| Attack adjustment | `0x1A0970` | `0x0D5F40` |
| World constructor | `0x18EB00` | `0x0C8570` |

The following were compared between client and server and have the same instruction sequence, immediates, displacements and widths, differing only in absolute addresses: the fatal-loot helper, the ground drop, the rarity roll, level variation, inverse curve, pet-food and formula generators, power scale, item clear, offset conversion, fixed addition, truncation helper, drop-record initialiser, item copy, column block accessor, zone item append, zone-set insertion, spawn constructor, both spawn destructors, element-buffer release, maximum-health routine, slot bonus, damageable predicate and attack adjustment. The hit routine, curve, resident lookup and generators P and Q were compared in the earlier contract. For the attack routine and the world update routine only the windows around each call site in section 6.1 were compared. The zone constructor, zone generation and world constructor were read in both images at the cited stores but not compared as wholes. Import slots used: operator delete at `0x2FC3F0` / `0x1582D0`; `MSVCR110.dll!_libm_sse2_pow_precise` (the double-precision power routine) at `0x2FC454` / `0x1582F8`.

Binary32 constants used below (client / server RVA, bits; values agree in both images): 1 at `0x345DC0` / `0x1586DC` (`0x3F800000`); 2 at `0x345E10` / `0x158708` (`0x40000000`); 32767 at `0x2FD338` / `0x158834` (`0x46FFFE00`); 360 at `0x2FCE6C` / `0x158780` (`0x43B40000`); 65536 at `0x2FCE7C` / `0x15878C` (`0x47800000`); 0.75 at `0x2FCDC4` / `0x1586D4` (`0x3F400000`); 3 at `0x2FCD70` / `0x158718` (`0x40400000`); nearest binary32 to 0.8 at `0x2FCDC8` / `0x1586D8` (`0x3F4CCCCD`); 10 at `0x345E74` / `0x15873C` (`0x41200000`); the variation ceiling `0x3F7FF972` at `0x302A3C` / `0x15AC58`; 20 at `0x2FCD74` / `0x15874C` (`0x41A00000`). Immediates: the drop scale base `0x3D924925` (nearest binary32 to 1/14), the loot scales 1 (`0x3F800000`) and 0.75 (`0x3F400000`), and the variation step `0x3D4CCCCD`.

## 3. Shared definitions

**Draw arithmetic.** Unless stated otherwise, "r mod n" is the signed 32-bit remainder of a draw by n, with the sign of the dividend (draws are never negative, so it is the ordinary remainder). An **offset draw** produces the binary32 value 1 minus ((draw converted to binary32, times 2) divided by 32767), each operation in binary32. A **rotation draw** produces (draw converted to binary32, times 360) divided by 32767, in binary32.

**Offset pair and drop position.** An offset pair is two offset draws, x then y. Each value, and a z value of positive zero, is multiplied by 65536 in binary32 and truncated toward zero to signed 64 bits (the truncation helper rounds with x87, then corrects toward zero). The drop position is the target's three signed 64-bit raw coordinates at `+0x10`, `+0x18`, `+0x20`, each plus its offset with 64-bit wrapping addition. All loot paths compute the offsets this way. Some go through the offset-to-fixed conversion and some do it inline; the arithmetic is the same.

**Item layout used here.** Items are `0x118` bytes. They hold a category byte at `+0x00`, a subtype byte at `+0x01`, 32-bit words at `+0x04` and `+0x08`, a rarity byte at `+0x0C`, a material byte at `+0x0D`, a byte at `+0x0E`, a signed 16-bit level at `+0x10`, 256 extension bytes at `+0x14` and an extension count at `+0x114`. **Item clear** writes zero to the category and subtype (one 16-bit store), `+0x04`, `+0x08`, the 16-bit `+0x0C` (rarity and material), `+0x0E`, the 256 extension bytes and the count, and writes 1 to the level. Bytes `+0x02`, `+0x03`, `+0x0F`, `+0x12`, `+0x13` are not written.

**Level curve.** The level-to-fraction curve of a binary32 L is 1 minus 1 divided by ((L minus 1) times the nearest binary32 to 0.05, plus 1), in binary32. Its inverse of a binary32 x is ((1 divided by (1 minus x)) minus 1) times 20, plus 1, in binary32. Both return through the x87 stack as exact binary32 loads.

**Power scale** of a binary32 level L and a signed 32-bit rarity R: first the curve of L is computed inline and multiplied by 3. Two binary32 factors follow. The first is 2 raised to that value, the second 2 raised to (R converted to binary32 times 0.25). Each power is evaluated by the double-precision power import on the binary32 value widened to double, and the result is rounded to binary32. The result is the first factor times the second, in binary32, returned through the x87 stack.

## 4. F9 virtual disposal

### 4.1 What the vector holds

The F9 resident is a zone record. The zone constructor stores the virtual table whose RTTI names the class `Zone` (`0x31E280` / `0x173CF8`). It zeroes the pointer vector at `+0x18`, `+0x1C`, `+0x20` and zeroes the 32-bit **base count** at `+0xA0` (client `0x22415B–0x224271`; server `0x148BCB–0x148CE1`). Zone generation stores the vector's element count into `+0xA0` once generation has added its records (client `0x1EDD5E`; server `0x121B3E`). The zone destructor deletes every element through virtual slot 0 with argument 1.

The elements are spawn records of `0x10F0` bytes. Their constructor stores the virtual table at `0x31DBEC` / `0x17383C`, whose RTTI type descriptor (`0x3679E8` / `0x183A00`) carries the decorated name of class `Spawn` in namespace `cube`. This table has exactly one slot. Its only entry is the deleting destructor (`0x1ADDB0` / `0x0E11D0`); the next word is a floating-point constant. In each image, a byte scan found the type descriptor referenced only by its own object locator and base-class descriptor, so no class derives from it. The table itself is referenced only by the constructor and the destructor body. Every inspected site that appends to a `+0x18` pointer vector appends an object built shortly before by the spawn constructor. These include the generation appends in server `0x118630` and `0x0E28E0`. The fatal branch's own F11 writes the element words `+0x38` and `+0x3C`, which are spawn-record fields. An exhaustive enumeration of every writer of every zone's `+0x18` vector was not made, so this identification is strong but not exhaustive (see section 7).

### 4.2 The routine reached

For each nonnull element from index `+0xA0` upward, F9's call through slot 0 with argument 1 reaches the deleting destructor. In order, it:

1. Runs the destructor body. This re-stores the spawn virtual table. If the pointer at `+0x109C` is nonnull, that block is released with operator delete and no destructor of its contents is run. Then three vectors are released in this order: `+0x10BC` (begin), `+0x10AC`, `+0x10A0`. For each, a nonnull begin pointer has its storage released, and its begin, end and capacity words are zeroed. Last, if the vector at `+0xF6C` is nonnull, each of its 12-byte elements from begin (`+0xF6C`) to end (`+0xF70`) is visited. A nonnull element buffer is released and the element's three words are zeroed. Then the outer storage is released and its three words are zeroed.
2. Because bit 0 of the argument is set, releases the spawn record itself with operator delete, and returns the record pointer.

The pointer in the zone vector is not cleared by the destructor. The F9 resize then truncates the vector to the base count, or grows it with zero-filled elements, as the earlier contract states. Nothing else is touched. The destructor does not read or write the creature map, any actor, the zone's other members, the output aggregate or the changed-zone collection. In particular, an actor that was created from a removed spawn record remains in the creature map unchanged.

Draws and recursion: the destructor body and the deleting destructor call only operator delete and the element-buffer release, which itself calls only operator delete. There are no draws, no indirect calls and no path back to the hit routine. The F9 resize and everything below it, its growth callee included (`0x00D9D0` / `0x05F080`), also reach no draw, no indirect call and not the hit routine in either image. So F9 adds no draws to the branch, and the earlier contract's ordering stands: all F5 draws come before all F10 draws.

Client and server: the two destructors and the deleting destructors are byte-identical apart from the virtual-table address and call displacements; the release routines are byte-identical apart from call displacements.

### 4.3 Consequence for F11

F11 uses the target's home zone (target `+0x1B0`, `+0x1B4`) and spawn index (`+0x1B8`), bounds-checked against that zone's current element count. The home zone lies inside the eight-by-eight square disposed by F9. So when F9 disposal ran and the target's index is at or above the base count, the index is now out of range. F11 then writes nothing; that spawn record was deleted in F9. Below the base count, F11 writes as specified.

## 5. F5 fatal-loot helper

### 5.1 Interface and admission

The helper receives the world receiver and two stack arguments, the target actor pointer and the changed-zone collection (the hit routine's third argument). It removes 8 bytes of arguments and returns nothing. [Client `0x19B180–0x19C3D5`; server `0x0D2AE0–0x0D3D35`.]

It returns at once, without any draw, when target byte `+0x60` is 0 or 3, or when bit `0x0800` of the target's 16-bit word `+0x7E` is set. Otherwise it makes the stages below in order. Every draw named is made exactly where stated. The ground-drop routine (section 5.3) makes further draws of its own at each call.

### 5.2 Ordered stages

**L1. Discarded draw.** One draw is made and its value is not used.

**L2. Worn item.** A local list of item pointers is built in this order, adding each slot whose category byte is nonzero: `+0x990`, `+0xAA8`, `+0x530`, `+0x878`, `+0x760`, `+0x648`, `+0x418`. These are items inside the target. If the list is empty, L2 makes no draw. Otherwise:

1. One draw. If r mod 4 is nonzero and bit `0x0200` of target word `+0x7E` is clear, L2 ends.
2. Offset pair (two draws).
3. One draw; its unsigned remainder by the list length selects the slot.
4. One rotation draw.
5. Ground drop of the selected slot's item (the actor's item is copied, not removed), at the drop position, with that rotation and scale 1.

The list storage is released at the helper's end.

**L3. Weapon or armour, then pet food.** Only when target byte `+0x60` is 1 or target 32-bit `+0x64` equals 143:

1. One draw. If r mod 10 is nonzero, L3 ends.
2. One draw; p1 is its parity. The level L is target signed 32-bit `+0x190` minus 1 plus p1 (wrapping), raised to 1 if below 1.
3. One draw; p2 is its parity.
4. Rarity roll with the target's byte `+0x1A8` (zero-extended) as base and flag zero (section 5.4; four draws).
5. If p2 is odd, generator P; if even, generator Q. Either is called with a local destination, level L, the rolled rarity and selector −1 (section 5.4).
6. Level variation of the generated item with step `0x3D4CCCCD` and the widening flag set (section 5.4; zero or one draw).
7. Offset pair, one rotation draw, then ground drop of the item with scale 1.
8. One draw. If r mod 5 is nonzero, L3 ends. Otherwise: rarity roll with the same base and flag zero (four draws), then the pet-food generator with the same destination, level L and that rarity (one draw; section 5.4), then level variation as in step 6. The pet-food item has level 1, so this variation makes no draw. Then offset pair, one rotation draw, and ground drop with scale 1.

**L4. Ingredients.** One draw: the count is r mod 3 plus 1. The loop then runs up to that many times. Each pass clears a local item, sets its level to 1 and its scale to 0.75, then selects by target 32-bit `+0x64` (the race) through a byte table and a jump table indexed by race minus 58 for races 58 to 139. [Tables client `0x19C424` / `0x19C3D8`, server `0x0D3D84` / `0x0D3D38`; index bytes identical; targets at the same offsets from each routine's start.] The category written is always 11. The other fields follow this table; a field marked "–" keeps its cleared value.

| Race(s) | Subtype | Material (`+0x0D`) | Rarity (`+0x0C`) | Scale |
| --- | --- | --- | --- | --- |
| 58 | 2 | 19 | – | 0.8 |
| 70 | 18 | – | – | 0.8 |
| 71 | 21 | – | – | 0.8 |
| 120, 121, 124, 126 | one draw: odd 1, even 5 | odd 2, even 21 | – | 0.8 |
| 122 | 27 | 0 | – | 0.8 |
| 123 | 11 | 27 | – | 0.8 |
| 125 | 6 | 0 | – | 0.8 |
| 127 | 20 | – | – | 0.8 |
| 128 | 23 | – | – | 0.8 |
| 131 | 0 | 11 | – | 3 |
| 132 | 0 | 1 | – | 3 |
| 133 | 0 | 12 | – | 3 |
| 134 | 0 | 17 | – | 0.8 |
| 135 | 0 | 13 | 1 | 3 |
| 136 | 0 | 14 | 2 | 3 |
| 137 | 0 | 15 | 3 | 3 |
| 138 | 0 | 16 | 4 | 3 |
| 139 | 8 | 22 | – | 0.8 |

For any other race, including those inside the table's range that map to its default, the category stays 0 and the loop ends at once. Only the count draw has been made then, and the remaining passes are not run. For a listed race, after the table (and its parity draw for races 120, 121, 124, 126) each pass makes an offset pair and one rotation draw, then a ground drop of the item with the pass's scale.

**L5. Formula, coins or a category-14 item.** Only when target byte `+0x60` is 1 or target `+0x64` equals 143; otherwise the helper goes to L6.

1. One draw. If r mod 50 is zero: offset pair, then one rotation draw, then the rarity roll with base (target byte `+0x1A8` plus 1) and flag zero (four draws). The formula generator follows with a local destination, the target's `+0x190` as level and that rarity (section 5.4). Last comes a ground drop of the formula item with scale 0.75. The rotation draw is made before the rarity roll and generator draws.
2. Then, if bit `0x0008` or `0x0010` of target word `+0x7E` is set, the category-14 path runs; otherwise the coin path.

*Coin path.* The power scale of (target `+0x190` converted to binary32, target byte `+0x1A8`) is stored as binary32 and multiplied by 10. One draw gives the factor (draw times 2, divided by 32767, plus 1) in binary32. The value is that factor times the scaled power. It is multiplied by 10 again if bit `0x0200` of `+0x7E` is set, and truncated toward zero to signed 32 bits. Out-of-range or NaN gives `0x80000000`. Let C be the value's remainder by 100, S the remainder by 100 of the value divided by 100, and G the value divided by 100 and again by 100 (all truncating). Then, in order: if C is nonzero, an item with category 12, subtype 0, material 10 and level C is cleared and filled. If S is nonzero, one with material 12 and level S. If G is nonzero, one with material 11 and level G. Levels are the low 16 bits. Each such item makes an offset pair and one rotation draw, then a ground drop with scale 0.75.

*Category-14 path.* One draw: v is r mod 1000 plus 20 times target byte `+0x1A8`. The grade is 0 if v is below 700, 1 if below 950, 3 if at least 999, otherwise 2. If bit `0x0200` of `+0x7E` is set, the grade is incremented and capped at 3. One further draw: if r mod 20 is nonzero, or the grade is 0, nothing more happens. Otherwise an item is cleared, its category byte set to 14 and its `+0x04` set to the raw value of one more draw. Its level gets the low 16 bits of target `+0x190` and its rarity byte the grade. It then makes an offset pair and one rotation draw, then a ground drop with scale 0.75.

**L6. Carried items.** Only when target byte `+0x60` is 1 (race 143 alone does not qualify). The target's container at `+0x11DC` is a vector of 12-byte bags, begin `+0x11DC`, end `+0x11E0`; the bag count is recomputed from those words before each bag. Each bag is a vector of `0x11C`-byte slots: a signed 32-bit quantity at slot `+0x00` and an item at slot `+0x04`. Bags and slots are visited in ascending order. Slots with category 0, quantity 0 or negative quantity are skipped. For other slots, a unit counter runs from 0 while it is below the quantity. Each unit whose item has category 1 and subtype 1 is skipped without draws. Every other unit makes an offset pair and one rotation draw, then a ground drop of the slot's item, copied unchanged, with scale 0.75. The container is not modified.

**L7. Return.** The worn-item list storage is released and the helper returns.

### 5.3 Ground drop (`0x19AEB0` / `0x0D2810`)

Input: the world receiver, an item, a drop position, a rotation, a scale, and the changed-zone collection. In order:

1. A local `0x148`-byte drop record is initialised: scale word `+0x134` = `0x3D924925`, byte `+0x138` = 0, `+0x13C` = 0, `+0x140` = 0, `+0x144` = −1, and the item's extension bytes and count cleared. The position goes to `+0x118–+0x12F` and the rotation argument to `+0x130`. The scale word is multiplied by the scale argument in binary32.
2. **One draw**: `+0x13C` becomes r mod 300 plus 500. `+0x144` becomes the world's 32-bit `+0x800160`.
3. Zone choice: each of x and y is the raw coordinate divided by 65536 (signed 64-bit, truncating), taken as 32 bits, then divided by 256 (truncating). The resident lookup is called with those. If it fails, the routine returns. Nothing is stored and no further draw is made, but the draw of step 2 has been consumed.
4. The item is copied into the record.
5. **One draw**: the record rotation `+0x130` is overwritten with a rotation draw. The rotation argument is therefore never kept.
6. Ground search. Block x and y are the raw x and y divided by 65536 (signed 64-bit, truncating). Block z is the raw z divided by 65536 the same way, minus 1 when the raw z is negative. From that z, the block at (x, y, z) is examined, and z is decremented while its type is 0 or 2. The type is the low five bits of the block's byte 3. The block examined is chosen in this order:
   - A static solid block (type 1) when x or y is negative or at least 2²⁴, or when the zone at (x divided by 256, y divided by 256) is absent.
   - The same when the column address is zero. That address is the zone's pointer at `+0xA8` plus ((y mod 256) times 256 plus (x mod 256)) times 32. Only a zero sum counts as absent.
   - The same when z is below the column's base at column `+0x10`.
   - A static type-0 block above the column top (column `+0x10` plus count `+0x1C`) when z is positive, and a static type-2 block there otherwise.
   - Inside the column, the stored block. If its type is 0, z is not positive and bit `0x40` of byte 3 is clear, the static type-2 block is used instead.

   So the search always stops at the latest at the column base. The static blocks are zero-filled data set by start-up initialisers: byte 3 is `0x01`, `0x00` and `0x82` for the solid, type-0 and type-2 blocks. [Server `0x184248`, `0x184244`, `0x184240`, set at `0x157140`, `0x157110`, `0x157170`; client `0x36B7D4`, `0x36B7D0`, `0x36B7CC`, set at `0x2FAB00`, `0x2FAAD0`, `0x2FAB30`.]
7. The record's z becomes (the final z plus 1) times 65536 as a signed 64-bit value.
8. The record is appended to the zone's item vector at zone `+0x30` (begin), `+0x34` (end), with `0x148`-byte elements.
9. The zone pointer is inserted into the changed-zone collection. That is an ordered tree of zone pointers with unsigned comparison; a zone already present is not added again.

The ground drop makes exactly two draws when the zone exists and one when it does not. It writes no actor and does not reach the hit routine.

### 5.4 Callees with draws

- **Rarity roll** (`0x1F8530` / `0x12BF40`; base B, flag F). There are always four draws, in order: v is the first draw mod (B plus 1). v is incremented when the second draw mod 100 is zero, when the third mod 1000 is zero, and when the fourth mod 10000 is zero. If F is nonzero, v becomes B plus 1. The result is v capped at 4.
- **Generators P and Q** with selector −1. Each group gate includes its group when the selector equals the group number or is negative, so −1 includes every group. Group contents and candidate rules are those of the earlier contract. P runs groups 1, 4, 2, 3 in that order. Their 5 + 3 + 3 + 4 = 15 draws give 3 + 3 + 2 + 3 = 11 candidates, and one selection draw follows: 16 draws. Q runs groups 1, 3, 2, 4 in that order, then its two always-present candidates. Their 4 + 5 + 5 + 5 + 2 + 2 = 23 draws give 4 + 5 + 5 + 5 + 1 + 1 = 21 candidates, and one selection draw follows: 24 draws. The selection draw's unsigned remainder by the candidate count picks the result. [Gates: P server `0x12C589`, `0x12C629`, `0x12C6B0`, `0x12C70E`; Q server `0x128C98`, `0x128D50`, `0x128E31`, `0x128F12`; the client gates sit at the same offsets in the instruction-identical client routines.]
- **Level variation** (`0x0C7E80` / `0x014470`; step d, widening flag). If the item's signed 16-bit level is below 2, nothing happens and no draw is made. Otherwise x is the curve of the level (converted to binary32) minus d. If x is less than or equal to zero (an ordered comparison; NaN continues), the level becomes 1 and no draw is made. Otherwise the low bound is the inverse curve of x, truncated toward zero and raised to at least 1. The high bound comes from the curve of the level plus d, capped at `0x3F7FF972` when greater: when the flag is set it is the inverse curve of that value, truncated toward zero; when the flag is clear it is the level itself. One draw follows, and the level becomes (r mod (high minus low plus 1)) plus low, stored as its low 16 bits. With d = `0x3D4CCCCD`, a level of exactly 2 gives a negative x, so a level-2 item becomes level 1 without a draw. A level of 3 or more makes one draw.
- **Pet-food generator** (`0x1F79E0` / `0x12B3F0`). It clears the destination as item clear does (it writes the subtype byte separately and leaves the category to its next store), then sets category 20 and level 1. **One draw**: the subtype is the low byte of the element selected by the draw's unsigned remainder by the element count of the world's 4-byte vector at `+0x88` / `+0x8C`. An empty vector faults with a division by zero. The level and rarity arguments are not used.
- **Formula generator** (`0x1F6D50` / `0x12A760`). It clears its destination, then makes **one draw**. Odd calls generator P, even calls generator Q, each with a local destination, the level, the rarity and selector −1. It copies the generated item into the destination, stores the generated item's category (zero-extended) in `+0x08`, and sets the category to 2. Draws: 1 + 16 or 1 + 24.

### 5.5 Draw count summary

For one helper call, in order (D is the ground drop's 1 or 2 draws, Var is 0 or 1 from level variation, and Gen is 16 for P or 24 for Q):

| Stage | Draws |
| --- | --- |
| Admission failed | 0 |
| L1 | 1 |
| L2 | 0 if no worn slot; 1 if the gate fails; else 1 + 2 + 1 + 1 + D |
| L3 (when its kind gate passes) | 1 if r mod 10 ≠ 0; else 1 + 1 + 1 + 4 + Gen + Var + 3 + D + 1, then, if the pet-food draw hits, 4 + 1 + 3 + D |
| L4 | 1 for the count, plus 3 + D for each pass with a listed race (4 + D for races 120, 121, 124, 126) |
| L5 (when its kind gate passes) | 1 for the formula draw, plus 2 + 1 + 4 + 1 + Gen + D on a formula; then either the coin path, 1 plus 3 + D per nonzero coin stack, or the category-14 path, 2 plus 1 + 3 + D when it produces an item |
| L6 (target byte `+0x60` = 1) | 3 + D per dropped unit |

All of these draws happen inside F5. They follow F1 to F4, which make no draws, and precede all F10 draws.

### 5.6 Effects and storage

The helper creates nothing outside the ground drop's appends. Every produced item becomes a `0x148`-byte record in the item vector at `+0x30` of the zone selected by its drop position, which need not be the target's home zone. Each affected zone is added to the changed-zone collection. The helper does not write the target or any other actor, the output aggregate, the creature map or any spawn record. It reaches no indirect call other than the security-cookie failure trap, and it does not reach the hit routine, the attack routine or the world update routine in either image.

Client and server: the helper, the ground drop and every loot callee in section 2's comparison list agree instruction by instruction. The client listing's only omission in the helper (a stack adjustment at `0x19C3B5`) was read from the image and matches. The ingredient tables and all constants agree in value. The static default blocks have the same byte-3 values in both images.

## 6. Upstream callers and the fatal test

### 6.1 Call sites

A byte scan of each image's code section for direct calls and jumps to the hit routine found exactly eight call sites in each image and no jumps. A scan of the whole image found no stored absolute reference to the routine, so there is no indirect entry. The fatal branch is inside the hit routine and is entered only through its admission test.

| Caller | Client site | Server site | Record passed |
| --- | --- | --- | --- |
| Attack routine, miss | `0x19700F` | `0x0D002F` | type byte `+0x45` = 3, damage = the attack's damage argument |
| Attack routine, block | `0x197229` | `0x0D0249` | type 1 |
| Attack routine, mana absorption | `0x1974DD` | `0x0D04FD` | type 5, damage = the attack's damage before absorption |
| Attack routine, main hit | `0x1986D0` | `0x0D16F0` | type 0 (16-bit store at `+0x45`), damage computed by the routine |
| World update, received hit list | `0x20CD9B` | `0x132B5B` | each received record unchanged |
| World update, projectile heal | `0x221E14` | `0x147BD4` | type 0; damage is a local amount with its sign flipped, halved when the actor tested at server `0x147B32` has byte `+0x60` equal to 1 (that actor's role was not traced) |
| World update, area heal (unattributed range) | `0x214BAC` | `0x13A96C` | type 0; damage = power × roll × 1 × −0.1 (binary32 `0xBDCCCCCD` at server `0x173CD0`) |
| World update, beam heal (unattributed range) | `0x214F72` | `0x13AD32` | type 0; damage = power × −2 (binary32 `0xC0000000` at server `0x1737F8`) × roll |

The heal records are initialised by the hit-record initialiser, so their type is 0. The roll in both area and beam heals is 1.25 minus (one draw times 0.5 divided by 32767), in binary32. "Power" is the value of the routine at `0x011AD0` (server), which was not audited. The two unattributed call sites use the world update routine's stack frame, including its output-aggregate and collection slots. They lie in a code range the call graph does not assign to any function, but they belong to that routine's body.

Conditions:

- **Attack routine** (seven call sites into it in each image: client `0x20DD18`, `0x20F45D`, `0x21476B`, `0x2166C8`, `0x2183DA`, `0x221E8C`, `0x222AB8`; server `0x133AD8`, `0x13521D`, `0x13A52B`, `0x13C488`, `0x13E19A`, `0x147C4C`, `0x148878`, all in the world update routine). It returns without any hit when the target's health is less than or equal to zero (an ordered comparison; NaN continues), or when world byte `+0xB4` is nonzero and the target is world `+0xB8`. At each of its four sites, the record is first appended to the output aggregate's hit list. The hit routine is then called only when world byte `+0xB4` is zero. The mana site is followed by the main-hit site when damage remains after absorption. The conditions that choose miss, block or absorption were not audited here.
- **Received hit list.** It runs when the incoming-updates argument is nonnull and its list (head at `+0x00`, count at `+0x04`) is nonempty. If world byte `+0xB4` is nonzero, the output aggregate's hit list (at `+0x00`) is cleared first. Then every received record is passed in list order, with no further gate.
- **Heals.** The hit routine is called when world byte `+0xB4` is zero, or when both the acting actor and the target are world `+0xB8`. The record is appended to the output aggregate's hit list first.

World byte `+0xB4` is written by the world constructor (`0x18EC0C` / `0x0C867C`) from its second argument. Each image constructs its world once (client `0x059F57`, server `0x149CBD`), and both calls pass zero. A search for other direct byte stores to that offset found only stores of zero. On this evidence the flag is zero in both programs. Indirect writes were not excluded.

### 6.2 What reaches the fatal test

In the hit routine (client `0x195A60`, server `0x0CEA80`; server addresses below), the paths that return before the fatal test are:

- the target lookup fails, or health is not greater than zero and not NaN (`0x0CEB0D`);
- world `+0xB8` is null and target byte `+0x60` is 0: the record goes to the output hit list instead (`0x0CEB1D–0x0CEB38`);
- target word `+0x7E` bit `0x2000` is set and no contribution entry with value greater than zero belongs to an actor with byte `+0x60` equal to 0 (`0x0CED07–0x0CED85`);
- type 3 (`0x0CED90`); type 1, which adds 0.25 to `+0x170` and caps it at 1 (`0x0CEE89–0x0CEEBF`); type 4, which calls the dodge routine at `0x008230` (`0x0CEEC4`);
- world byte `+0xB4` is nonzero and the target is not world `+0xB8` (`0x0CEEF2–0x0CEF07`).

Every other path reaches the test at `0x0CF153`. The only health writes are on three paths:

1. Damage greater than zero (ordered), type not 5, the target damageable, and the attacker absent or damageable: health minus damage (`0x0CF00E–0x0CF01B`). The **damageable** predicate (`0x03E310` / `0x009620`) holds for byte `+0x60` values 0, 1, 3 and 5. It holds for 6 except races 140, 141 and 142. It fails for every other value.
2. Damage less than zero (ordered): health minus damage (`0x0CF031–0x0CF047`).
3. On path 2 only, the clamp (`0x0CF04F–0x0CF072`).

The **non-damage paths** reach the test with no health write. They are: damage zero, negative zero or NaN with any type not listed above; damage greater than zero with type 5 (the mana-shield buff path, `0x0CEF25–0x0CEFE7`); and damage greater than zero when the target or attacker is not damageable. Before the test, the routine calls the creature-map find, the contribution-map insert, two attacker predicates, the attack adjustment (`0x1A0970` / `0x0D5F40`), the double-precision power import, and the buff-record append. On the heal path it also calls the enemy test (`0x196CA0` / `0x0CFCC0`) and the attack adjustment. Among all routines reachable from these, there is no store to `+0x16C`, no draw, no indirect call other than the security-cookie trap, and no path back to the hit routine. Entry required health greater than zero or NaN. **So a non-damage path cannot reach the fatal test with non-positive health in either image**, on this evidence.

On path 2, health only grows (for a finite negative damage) or stays NaN. A NaN maximum also leaves health unchanged, because the clamp comparison is ordered. So path 2 can enter the fatal branch only through the clamp.

### 6.3 Can the clamp store a non-positive value?

The clamp stores the maximum-health routine's result whenever stored health is greater than it (ordered). The routine (`0x044DB0` / `0x00FDA0`) computes, in binary32:

1. A = 2 raised to (3 times the curve of the level), with the level being signed `+0x190` converted to binary32 and the curve computed inline.
2. B = 2 raised to (rarity byte `+0x1A8` times 0.25). When byte `+0x60` is 0, B is instead 2 raised to 1, which is 2. Each power is evaluated in double precision from the binary32 value and rounded to binary32.
3. The base is B times A times the binary32 multiplier at `+0x178`.
4. The base is then multiplied by a class factor for byte `+0x140`: 1.3 (`0x3FA66666`) for class 1, 1.1 (`0x3F8CCCCD`) for class 2, 1.2 (`0x3F99999A`) for class 4, none otherwise. It is multiplied by a further 1.25 when byte `+0x140` is 1 and byte `+0x141` is 1. [Jump table client `0x044FE0`, server `0x00FFD0`; same targets relative to the routine start.]
5. A slot bonus is added for each of these slots, in order, when its category matches: `+0x990` with category 3, `+0xAA8` with 3, `+0x530` with 4, `+0x648` with 6, `+0x760` with 5, `+0x878` with 7.

The slot bonus (`0x0C70B0` / `0x013CE0`) is zero for categories other than 3 to 7. For those categories it is the power scale of (level plus 0.1 times extension count, rarity), times 5, times 0.5 (or 1 for category 4), times a factor between 1 and 3. That factor is 2 minus a value from 0 to 1 derived from the item's `+0x04`, plus 1, 0.5 or 0.75 for materials 1, 26 and 27. So each bonus is zero or positive, or infinite or NaN for extreme inputs.

So the result is non-positive only if the base is non-positive and the bonuses do not lift the sum above zero. A and B are never negative and B is at least 1. The base is therefore non-positive only when:

- the multiplier at `+0x178` is zero, negative or negative zero; or
- the level is exactly −19. The curve's denominator then rounds to zero, the curve is −∞, A is 0, and the base is zero, or NaN if the multiplier is infinite or NaN.

In those cases the clamp can store a non-positive value, and the fatal test then passes. Whether a running program ever has such a multiplier or level is not established. The routine makes no draws and reaches no other routine except the slot bonus, the power scale and the power import.

Client and server: the hit routine (per the earlier contract), the attack routine's four call contexts, the world update routine's four call contexts, the maximum-health routine, the slot bonus and the damageable predicate agree instruction by instruction. Their constants and jump tables agree in value.

## 7. Unresolved

- The F9 element type rests on the evidence of section 4.1: a single-slot spawn table, no derived class, and every inspected append site. Not every writer of every zone's `+0x18` vector was enumerated.
- The type and fate of the block at spawn `+0x109C`, released without a destructor. It is not established whether that leaks owned storage.
- The world vector at `+0x88` used by the pet-food generator: what fills it, and whether it can be empty (a division fault).
- How the zone item vector, the zone's base count and the changed-zone collection are later consumed, saved or transmitted, and whether the base count is restored when a zone is loaded.
- What the 32-bit `+0x04` of a category-14 item, the categories, subtypes, materials, races and grades mean in the game. The game meaning of any field is not asserted.
- The value of the power routine used by area and beam heals (server `0x011AD0`), and so whether a heal record's damage can ever be zero or positive. The derivation of the attack routine's main-hit damage and of the projectile heal's amount is also open.
- The conditions inside the attack routine that select miss, block and absorption, the conditions at its seven call sites, and the callers of the world update routine.
- Whether actor multiplier `+0x178` can be non-positive or level `+0x190` can be exactly −19 at run time. These are the only routes by which the maximum-health clamp stores a non-positive value.
- Indirect writes to world byte `+0xB4`, and the run-time value of world `+0xB8`, in each program.
- Health writes through addressing forms other than a plain base plus `+0x16C` were not searched. The negative results of section 6.2 also rest on the call graph.
- Padding and unwritten bytes of drop records and items, floating-point control state, and concurrency or actor lifetime during the branch.
