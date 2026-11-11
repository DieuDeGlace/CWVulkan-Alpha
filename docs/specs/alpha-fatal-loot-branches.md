# Fatal-loot helper: branches preceding the inventory-unit branch

Status: FINAL / STABLE / HELD.

This independently authored, code-free contract covers the fatal-loot helper from its entry up to, but not including, the inventory-unit branch. The inventory-unit branch and the nested world-drop helper are the subject of the sibling contract `alpha-fatal-world-drop-production.md`; that document was used only to locate the two routines, and nothing in it was taken as evidence here. The Infinite-Cubes reconstruction was not consulted.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`; every address below is an RVA. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method and client/server agreement

The instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt` were read for every routine named in this contract. For every such routine, in each image separately, the byte column of the listing was checked against the bytes of the image itself at the same address; no mismatch was found. The listings omit a few alignment-padding bytes and, in the client only, one three-byte stack-adjust instruction after the storage release at the end of each of the two large generators; those bytes were read from the image and are accounted for. Decompiler output under `decompiled/*/functions/` was used as a lead only.

Client and server were then compared instruction by instruction. For the fatal-loot helper (883 instructions from entry up to the first instruction of the inventory-unit branch) and for every nested routine listed in section 2, the two images contain the same instruction sequence with the same immediates, displacements, widths and branch shapes; they differ only in the absolute addresses of callees, import slots, constants, the security cookie and the exception-handler record. Each differing address pair was resolved in its own image (callee to callee, constant to constant) and the constants' bytes were read from each image. The two switch tables of the kind-specific branch were read from each image and are identical after rebasing. The conclusion "client and server agree" in this contract therefore rests on each image's own bytes, not on an assumption that they were built from the same source.

## 2. Routines and constants

| Role in this contract | Client RVA | Server RVA | Audit state |
| --- | --- | --- | --- |
| Fatal-loot helper (entry) | `0x19B180` | `0x0D2AE0` | audited to the branch boundary |
| Start of inventory-unit branch (end of scope) | `0x19C163` | `0x0D3AC3` | boundary only |
| Nested world-drop helper | `0x19AEB0` | `0x0D2810` | call sites only; body not re-audited here |
| Pointer-list append | `0x26ADD0` | `0x0F2BE0` | audited, except its growth callee |
| Offset triplet to raw triplet | `0x02C460` | `0x002510` | audited |
| Raw triplet sum | `0x02C800` | `0x002CB0` | audited |
| Item reset | `0x02F3E0` | `0x006FD0` | audited |
| Grade draw | `0x1F8530` | `0x12BF40` | audited |
| Generator P | `0x1F8AD0` | `0x12C4E0` | audited, except candidate-list growth |
| Generator Q | `0x1F51E0` | `0x128BF0` | audited, except candidate-list growth |
| World-list item producer | `0x1F79E0` | `0x12B3F0` | audited; the list's contents are not |
| Wrapped-item producer | `0x1F6D50` | `0x12A760` | audited |
| Level adjuster | `0x0C7E80` | `0x014470` | audited |
| Level-to-fraction curve | `0x03CA60` | `0x007D60` | audited |
| Fraction-to-level curve | `0x0462F0` | `0x011090` | audited |
| Value curve | `0x045F60` | `0x010F00` | audited, except the power import |
| Candidate append / item copies | `0x0C7830`, `0x00EE70`, `0x02C5E0` | `0x128530`, `0x013710`, `0x002A70` | audited (no draws) |
| Float-to-integer conversion | `0x28D946` | `0x14A946` | audited; byte-identical in both images |
| Power import thunk | `0x28D8D4` | `0x14AB14` | import identity only |

The random source in every routine above is the import slot at client RVA `0x2FC468` and server RVA `0x1582F4`; in each image the slot's import-name entry reads `rand`. The power import slot is client RVA `0x2FC454`, server RVA `0x1582F8`; in each image its name entry reads `_libm_sse2_pow_precise`.

Binary32 constants read from the images (client RVA / server RVA, bit pattern, value):

| Value | Client | Server | Bits |
| --- | --- | --- | --- |
| 1 | `0x345DC0` | `0x1586DC` | `0x3F800000` |
| 2 | `0x345E10` | `0x158708` | `0x40000000` |
| 32767 | `0x2FD338` | `0x158834` | `0x46FFFE00` |
| 360 | `0x2FCE6C` | `0x158780` | `0x43B40000` |
| 65536 | `0x2FCE7C` | `0x15878C` | `0x47800000` |
| 0.75 | `0x2FCDC4` | `0x1586D4` | `0x3F400000` |
| 3 | `0x2FCD70` | `0x158718` | `0x40400000` |
| nearest binary32 to 0.8 | `0x2FCDC8` | `0x1586D8` | `0x3F4CCCCD` |
| 10 | `0x345E74` | `0x15873C` | `0x41200000` |
| nearest binary32 to 0.05 | `0x2FCDB4` | `0x1586B8` | `0x3D4CCCCD` |
| 20 | `0x2FCD74` | `0x15874C` | `0x41A00000` |
| upper fraction cap (just below 1) | `0x302A3C` | `0x15AC58` | `0x3F7FF972` |
| 0.25 | `0x345D98` | `0x1586C8` | `0x3E800000` |

One binary64 constant is used: 2, bits `0x4000000000000000`, at client RVA `0x345E18` and server RVA `0x1588D8`. Three binary32 values are instruction immediates rather than data: `0x3F800000` (1), `0x3F400000` (0.75) and `0x3D4CCCCD` (0.05).

All floating-point steps described below are separate scalar SSE operations in the stated order, in binary32 unless binary64 is stated. Draw results are converted from signed 32-bit integer to binary32. The contract assumes the default round-to-nearest SSE state; the control state is not set or checked by these routines.

## 3. Shared vocabulary

**Actor fields tested.** The helper tests only these members of the actor record it is given; their game meaning is not asserted here.

- byte at `+0x60`: compared for equality with 0, 1 and 3;
- 32-bit value at `+0x64` ("kind"): compared for equality with 143, and range-mapped in the kind-specific branch;
- 16-bit word at `+0x7E` ("flags"): bit `0x0800`, bit `0x0200`, and the pair of bits `0x0008` and `0x0010`;
- 32-bit value at `+0x190` ("level value"): used as a signed 32-bit integer, and in places only its low 16 bits;
- byte at `+0x1A8` ("tier byte"): always zero-extended;
- three raw signed 64-bit coordinates starting at `+0x10`;
- seven item records at `+0x418`, `+0x530`, `+0x648`, `+0x760`, `+0x878`, `+0x990` and `+0xAA8` (spacing `0x118`).

**Item record.** An item occupies `0x118` bytes: category byte `+0x00`; subtype byte `+0x01`; 32-bit `+0x04`; 32-bit `+0x08`; bytes `+0x0C`, `+0x0D`, `+0x0E`; 16-bit level word `+0x10`; 32 eight-byte entries from `+0x14`; 32-bit entry count `+0x114`. Bytes `+0x02`, `+0x03`, `+0x0F`, `+0x12` and `+0x13` are never written by any routine in this contract and their content in produced items is not established.

**Item reset** (`0x02F3E0` / `0x006FD0`, and the same writes made inline where stated): category and subtype zero, `+0x04` and `+0x08` zero, bytes `+0x0C`, `+0x0D`, `+0x0E` zero, entry count zero, the 256 entry bytes zero, level word 1.

**Parity of a draw** means the signed remainder of the draw modulo 2 being nonzero ("odd") or zero ("even"). **Draw modulo N** means the signed 32-bit remainder with truncating division unless "unsigned" is stated. The documented range of the random source is 0 through 32767, so signed remainders are nonnegative in ordinary operation; the code's signed forms are stated because they are what the instructions do. The random source's own implementation is outside this contract.

**Horizontal offset draw.** Convert the draw to binary32; multiply by 2; divide by 32767; subtract the quotient from 1. The result lies in the closed interval from −1 to 1.

**Angle draw.** Convert the draw to binary32; multiply by 360; divide by 32767.

**Placement.** A branch makes an X offset draw, then a Y offset draw, and uses binary32 positive zero as the Z offset. Each of the three offsets, in X, Y, Z order, is multiplied by 65536, stored as binary32, loaded onto the x87 stack and converted to a signed 64-bit integer by the conversion routine (`0x28D946` / `0x14A946`), which truncates toward zero for ordinary finite values. The three integers are added to the actor's three raw 64-bit coordinates at `+0x10` (low-word addition with carry into the high word) to form the position triplet. Most branches do this through the two small routines `0x02C460` / `0x002510` (conversion) and `0x02C800` / `0x002CB0` (sum); two branches do it inline with identical arithmetic. In the routine form the conversion runs immediately after the Y draw and the actor coordinates are read just before the nested drop call; in the inline form the coordinates are read immediately after the conversion, before the angle draw. No game routine runs between those points, so the difference is observable only under concurrent mutation, which is not addressed.

**Nested drop call.** Every drop in this contract calls the nested world-drop helper with the world receiver the fatal-loot helper itself received, and with these arguments in order: item address, address of the position triplet, the binary32 angle value, a binary32 scale value, and the collection pointer the fatal-loot helper received as its second argument. The nested helper's own draws and effects (one or two further draws per call, per the sibling contract) are not restated or re-verified here; "nested draws" below refers to them.

## 4. Entry, admission and the discarded draw

The helper receives the world receiver, the actor pointer and a collection pointer. [Client `0x19B1AE–0x19B1BA`; server `0x0D2B0E–0x0D2B1A`.]

Admission, in order: if the actor's `+0x60` byte is 0 the helper returns; if it is 3 the helper returns; if flags bit `0x0800` is set the helper returns. A rejected call makes no draw and has no other effect. [Client `0x19B1C0–0x19B1E2`; server `0x0D2B20–0x0D2B42`.]

An admitted call immediately makes one draw whose result is not used for anything. [Client `0x19B1EE`; server `0x0D2B4E`.]

The branches then run in the fixed order of sections 5 to 10. Each branch is evaluated independently of whether an earlier branch dropped anything, except where a gate is explicitly shared.

## 5. Worn-item branch

**Candidate list.** A local list of item addresses is built with no draws. The seven actor item records are examined in this order: `+0x990`, `+0xAA8`, `+0x530`, `+0x878`, `+0x760`, `+0x648`, `+0x418`. A record whose category byte is nonzero has its address appended, preserving that order. The list's storage is released when the whole helper returns (after the inventory-unit branch). [Client `0x19B1F0–0x19B321`, release `0x19C3A5–0x19C3B5`; server `0x0D2B50–0x0D2C81`; append routine `0x26ADD0` / `0x0F2BE0`.]

**Gate.** If the list is empty, the branch makes no draw. Otherwise one draw is made. The branch proceeds when that draw modulo 4 is zero, or when flags bit `0x0200` is set; otherwise it ends. The draw is consumed whenever the list is nonempty, whatever the flag. [Client `0x19B326–0x19B351`; server `0x0D2C86–0x0D2CB1`.]

**Drop.** In order: X offset draw; Y offset draw; conversion of the offsets; one selection draw, reduced by unsigned 32-bit division by the number of list entries, the remainder being the list index; one angle draw; coordinate sum; nested drop call. The item argument is the selected list entry, that is, the address of the actor's own item record, not a copy. The scale argument is 1 (`0x3F800000`). The selection draw sits between the Y draw and the angle draw. The helper does not clear, alter or mark the selected actor record. [Client `0x19B357–0x19B422`; server `0x0D2CB7–0x0D2D82`.]

Direct draws: none with an empty list; one when the gate fails; five plus nested draws when it passes.

## 6. Generated-item branch

**Gate.** The branch runs only when the actor's `+0x60` byte equals 1 or its kind equals 143. Otherwise no draw is made and control passes to section 7. When it runs, one draw is made; the branch continues only if that draw modulo 10 is zero. [Client `0x19B427–0x19B44B`; server `0x0D2D87–0x0D2DAB`.]

**First item.** In order:

1. One draw; its parity contributes 1 (odd) or 0 (even). The item level is the actor's level value minus 1 plus that contribution, raised to 1 if the signed result is below 1.
2. One draw; its parity selects the generator: odd selects generator P, even selects generator Q.
3. The grade draw routine is called with the tier byte and a zero force flag (four draws; section 11.1).
4. The selected generator is called with the level from step 1, the grade from step 3 and a selector of −1 (section 11.2 or 11.3), filling a local item.
5. The level adjuster is applied to that item with a delta of 0.05 (`0x3D4CCCCD`) and its widening flag set (zero or one draw; section 11.4).
6. X offset draw, Y offset draw, conversion, angle draw, coordinate sum, nested drop call with the local item and scale 1 (`0x3F800000`).

[Client `0x19B451–0x19B590`; server `0x0D2DB1–0x0D2EF0`.]

**Second item.** After the first item's drop, one draw is made; the second item is produced only if that draw modulo 5 is zero. In order: the grade draw routine is called with the tier byte and a zero force flag (four draws; its result is passed on but has no effect on the item); the world-list item producer fills the local item (one draw; section 11.5); the level adjuster is applied with delta 0.05 and the widening flag set, which makes no draw and changes nothing because the producer leaves the level word at 1; then X offset draw, Y offset draw, conversion, angle draw, coordinate sum, nested drop call with scale 1. [Client `0x19B595–0x19B696`; server `0x0D2EF5–0x0D2FF6`.]

Direct caller draws around the generators: one gate draw; if passed, two draws before the grade routine and three placement draws after the level adjuster, then one draw for the second-item test, and three more placement draws if that test passes.

## 7. Kind-specific branch

This branch has no admission gate of its own: every admitted actor reaches it.

**Count.** One draw is made; the item count is that draw modulo 3, plus 1. [Client `0x19B6A0–0x19B6BF`; server `0x0D3000–0x0D301F`.]

**Per item.** A local item is reset inline (level word 1) and a local scale is set to 0.75. The kind minus 58 is compared unsigned with 81. If it is greater, or if the kind maps to no row below, the branch ends at once: the remaining count is abandoned and no further draw is made by this branch. Otherwise the row for the kind is applied. [Client `0x19B6D0–0x19B9EA`, target table `0x19C3D8`, index table `0x19C424`; server `0x0D3030–0x0D334A`, tables `0x0D3D38` and `0x0D3D84`.]

| Kind | Category | Subtype | Byte `+0x0C` | Byte `+0x0D` | Scale |
| --- | --- | --- | --- | --- | --- |
| 58 | 11 | 2 | 0 | 19 | 0.8 |
| 70 | 11 | 18 | 0 | 0 | 0.8 |
| 71 | 11 | 21 | 0 | 0 | 0.8 |
| 120, 121, 124, 126 | 11 | see below | 0 | see below | 0.8 |
| 122 | 11 | 27 | 0 | 0 | 0.8 |
| 123 | 11 | 11 | 0 | 27 | 0.8 |
| 125 | 11 | 6 | 0 | 0 | 0.8 |
| 127 | 11 | 20 | 0 | 0 | 0.8 |
| 128 | 11 | 23 | 0 | 0 | 0.8 |
| 131 | 11 | 0 | 0 | 11 | 3 |
| 132 | 11 | 0 | 0 | 1 | 3 |
| 133 | 11 | 0 | 0 | 12 | 3 |
| 134 | 11 | 0 | 0 | 17 | 0.8 |
| 135 | 11 | 0 | 1 | 13 | 3 |
| 136 | 11 | 0 | 2 | 14 | 3 |
| 137 | 11 | 0 | 3 | 15 | 3 |
| 138 | 11 | 0 | 4 | 16 | 3 |
| 139 | 11 | 8 | 0 | 22 | 0.8 |

Kinds 59 to 69, 72 to 119, 129 and 130, and every kind below 58 or above 139, have no row. The scales are the binary32 constants `0x3F4CCCCD` and `0x40400000` of section 2. For kinds 120, 121, 124 and 126 one extra draw is made for each item before its placement draws: an odd draw gives subtype 1 with byte `+0x0D` equal to 2; an even draw gives subtype 5 with byte `+0x0D` equal to 21. All other item fields keep their reset values, so the level word is 1 and `+0x04`, `+0x08`, `+0x0E` and the entry area are zero.

**Drop.** For each item: X offset draw, Y offset draw, inline conversion and coordinate sum, angle draw, nested drop call with the local item and the row's scale. The item counter then advances; the loop repeats while the counter is below the count. [Client `0x19B9F0–0x19BB22`; server `0x0D3350–0x0D3482`.]

Direct draws: one for a kind with no row; otherwise one, plus for each of the one to three items three draws (four for kinds 120, 121, 124, 126) and nested draws.

## 8. Shared gate for the remaining branches

The branches of sections 9 and 10 run only when the actor's `+0x60` byte equals 1 or its kind equals 143. When neither holds, control passes directly to the inventory-unit branch with no draw. [Client `0x19BB28–0x19BB35`; server `0x0D3488–0x0D3495`.]

## 9. Wrapped generated-item branch

One draw is made; the branch continues only if that draw modulo 50 is zero. [Client `0x19BB3B–0x19BB47`; server `0x0D349B–0x0D34A7`.]

When it continues, in order: X offset draw; Y offset draw; inline conversion and coordinate sum; angle draw; then the grade draw routine is called with the tier byte plus 1 (computed after zero-extension, so 1 to 256) and a zero force flag (four draws); then the wrapped-item producer is called with the actor's level value and that grade (section 11.6: one draw, then a full generator P or generator Q run with selector −1); then the nested drop call with the produced item and scale 0.75 (`0x3F400000`). All three placement draws therefore precede the grade and generator draws, which in turn precede the nested draws. The level adjuster is not applied in this branch; the item's level word is the low 16 bits of the actor's level value. [Client `0x19BB4D–0x19BC87`; server `0x0D34AD–0x0D35E7`.]

Whether or not the modulo-50 test passes, control continues to section 10.

## 10. Flag-selected final branch

The low byte of the flags word is tested against `0x18`. If neither bit `0x0008` nor bit `0x0010` is set, section 10.1 runs; if either is set, section 10.2 runs. Exactly one of the two runs. [Client `0x19BC8F–0x19BC95`; server `0x0D35EF–0x0D35F5`.]

### 10.1 Counted-denomination branch (neither bit set)

**Amount.** The value curve (section 11.7) is evaluated with the actor's level value, converted from signed 32-bit to binary32, and the tier byte. Its binary32 result is multiplied by 10 to give the base. One draw is then made and converted to binary32; it is multiplied by 2, divided by 32767, increased by 1, and multiplied by the base. If flags bit `0x0200` is set, the product is multiplied by 10 again. The result is converted to a signed 32-bit integer with truncation toward zero; call it the amount. [Client `0x19BC9B–0x19BD16`; server `0x0D35FB–0x0D3676`.]

**Split.** With truncating signed division: the low part is the amount modulo 100; the middle part is the amount divided by 100, then taken modulo 100; the high part is the amount divided by 10000 (computed as two successive divisions by 100). [Client `0x19BD1A–0x19BD42`; server `0x0D367A–0x0D36A2`.]

**Drops.** Up to three items are produced, in this fixed order, each only if its part is nonzero:

1. low part: byte `+0x0D` equal to 10;
2. middle part: byte `+0x0D` equal to 12;
3. high part: byte `+0x0D` equal to 11.

Each item is reset by the item reset routine, given category 12 and subtype 0, the stated `+0x0D` byte, and a level word equal to the low 16 bits of its part. Each then makes an X offset draw, a Y offset draw, conversion, an angle draw, coordinate sum and a nested drop call with scale 0.75 (`0x3F400000`). A zero part makes no draw. [Client `0x19BD48–0x19BFF6` and shared call tail `0x19C156–0x19C15E`; server `0x0D36A8–0x0D3956`, `0x0D3AB6–0x0D3ABE`.]

Direct draws: one, plus three and nested draws per nonzero part. Behaviour for a nonpositive or out-of-range amount is whatever the stated integer steps give; no clamp exists and such inputs were not examined further.

### 10.2 Tiered-item branch (either bit set)

**Tier.** One draw is made. A score is formed as that draw modulo 1000 plus 20 times the tier byte. With signed comparisons: a score below 700 gives tier 0; otherwise a score below 950 gives tier 1; otherwise a score below 999 gives tier 2; otherwise tier 3. If flags bit `0x0200` is set, the tier is increased by 1 and limited to 3. [Client `0x19BFFB–0x19C058`; server `0x0D395B–0x0D39B8`.]

**Gate.** A second draw is always made. The branch continues only if that draw modulo 20 is zero and the tier is nonzero. [Client `0x19C05E–0x19C076`; server `0x0D39BE–0x0D39D6`.]

**Item and drop.** The local item is reset by the item reset routine and its category byte set to 14 (subtype stays 0). One draw is made and its unreduced 32-bit result is stored at item `+0x04`. The level word is set to the low 16 bits of the actor's level value, and byte `+0x0C` to the tier. Then X offset draw, Y offset draw, conversion, angle draw, coordinate sum, nested drop call with scale 0.75 (`0x3F400000`). [Client `0x19C07C–0x19C15E`; server `0x0D39DC–0x0D3ABE`.]

Direct draws: two when the gate fails; six plus nested draws when it passes.

After section 10, control reaches the inventory-unit branch.

## 11. Nested routines

### 11.1 Grade draw (`0x1F8530` / `0x12BF40`)

Inputs: a 32-bit integer and a byte force flag. The receiver is not used. Let the bound be the integer plus 1. Four draws are always made, in order: the first modulo the bound is the starting grade; the second, third and fourth each add 1 to the grade when the draw modulo 100, modulo 1000 and modulo 10000 respectively is zero. If the force flag is nonzero the grade is replaced by the bound. The result is the grade limited to at most 4 by signed comparison. Every call in this contract passes a zero force flag.

### 11.2 Generator P (`0x1F8AD0` / `0x12C4E0`)

Inputs: destination item, a level (low 16 bits used), a grade (low 8 bits used) and a signed selector. The receiver is not used. It builds a list of candidate items, makes one final draw, and copies the candidate whose index is that draw reduced by unsigned division by the candidate count into the destination. Every candidate has category 3, the supplied level word, the supplied grade at byte `+0x0C`, zero at `+0x08` and `+0x0E`, no entries, and a `+0x04` value equal to its own draw modulo 100 (called its modifier draw below).

A group is included when the selector equals the group's number or is negative; every call in this contract passes −1, so all groups are included, in this order:

- group 1: one draw modulo 3 gives the subtype (0 to 2), then a modifier draw, byte `+0x0D` equal to 1; then one draw modulo 3 plus 15 gives the subtype (15 to 17), then a modifier draw, byte `+0x0D` equal to 1; then subtype 13 with a modifier draw, byte `+0x0D` equal to 1;
- group 4: subtypes 3, 5 and 4 in that order, each with a modifier draw, byte `+0x0D` equal to 1;
- group 2: subtype 6 with a modifier draw; then one draw that is discarded; then subtype 8 with a modifier draw; byte `+0x0D` equal to 2 for both;
- group 3: subtype 10 with a modifier draw and subtype 11 with a modifier draw, byte `+0x0D` equal to 2 for both; then subtype 12, for which one draw's parity sets byte `+0x0D` to 11 when odd and 12 when even, followed by its modifier draw.

With selector −1 this is 11 candidates and 17 draws including the final selection draw. [Client `0x1F8AD0–0x1F8E01`; server `0x12C4E0–0x12C814`.]

### 11.3 Generator Q (`0x1F51E0` / `0x128BF0`)

Same inputs, selection rule, level, grade and modifier handling as generator P. Every candidate has subtype 0. With selector −1 the candidates are created in this order, each with one modifier draw:

- byte `+0x0D` equal to 1: categories 7, 4, 5, 6;
- byte `+0x0D` equal to 25: categories 7, 7, 4, 5, 6;
- byte `+0x0D` equal to 26: categories 7, 7, 4, 5, 6;
- byte `+0x0D` equal to 27: categories 7, 7, 4, 5, 6;
- category 8, preceded by one draw whose parity sets byte `+0x0D` to 12 when odd and 11 when even;
- category 9, preceded by one draw with the same parity rule.

The first four lines correspond to selector values 1, 3, 2 and 4 respectively; the last two are unconditional. With selector −1 this is 21 candidates and 24 draws including the final selection draw. [Client `0x1F51E0–0x1F56B5`; server `0x128BF0–0x1290C8`.]

For both generators the candidate-list growth routine (client `0x0C6940`, server `0x013940`) was not audited; it is assumed to be a draw-free storage service.

### 11.4 Level adjuster (`0x0C7E80` / `0x014470`)

Inputs: the item, a binary32 delta and a byte widening flag. Let L be the item's level word read as signed 16-bit.

- If L is below 2, nothing happens and no draw is made.
- Otherwise the level-to-fraction curve is applied to L converted to binary32: subtract 1; multiply by 0.05 (`0x3D4CCCCD`); add 1; divide 1 by that; subtract the quotient from 1. The delta is subtracted from the result. If the difference is not greater than zero, the level word is set to 1 and no draw is made.
- Otherwise the lower bound is the fraction-to-level curve of that difference, truncated toward zero to a signed 32-bit integer and raised to 1 if below 1. The fraction-to-level curve is: subtract the input from 1; divide 1 by that; subtract 1; multiply by 20; add 1.
- The upper bound is L when the widening flag is zero. When it is nonzero, the level-to-fraction curve of L is recomputed, the delta is added, the sum is replaced by the cap `0x3F7FF972` if it exceeds the cap, and the upper bound is the fraction-to-level curve of that value truncated toward zero.
- One draw is made. The new level word is the lower bound plus the draw modulo (upper bound minus lower bound plus 1), stored as 16 bits.

A span of zero would fault and a negative span is not guarded; neither was shown reachable or unreachable. As an arithmetic consequence of the stated steps with delta 0.05, L equal to 2 yields a nonpositive difference and so becomes level 1 with no draw, while L of 3 or more reaches the draw. [Client `0x0C7E80–0x0C7F5C`, curves `0x03CA60`, `0x0462F0`; server `0x014470–0x01454C`, curves `0x007D60`, `0x011090`.]

### 11.5 World-list item producer (`0x1F79E0` / `0x12B3F0`)

Inputs: destination item and two further arguments that are not used. The destination is reset, its category set to 20, and one draw is made. The world receiver holds a list of 32-bit elements with begin pointer at receiver `+0x88` and end pointer at `+0x8C`; the draw is reduced by unsigned division by the element count, and the low byte of the selected element becomes the subtype. The level word stays 1. An empty list would fault; the list's contents, population and lifetime are not established here.

### 11.6 Wrapped-item producer (`0x1F6D50` / `0x12A760`)

Inputs: destination item, a level and a grade. The destination is reset. One draw is made: odd runs generator P, even runs generator Q, each with the supplied level and grade and selector −1, into a temporary item. The temporary item is copied to the destination; the destination's `+0x08` is then set to the copied category byte zero-extended to 32 bits, and its category byte is set to 2. Subtype, `+0x04`, bytes `+0x0C` to `+0x0E`, level word and the (empty) entry area are those of the generated item.

### 11.7 Value curve (`0x045F60` / `0x010F00`)

Inputs: a binary32 level and a 32-bit integer. In order: subtract 1 from the level; multiply by 0.05 (`0x3D4CCCCD`); add 1; divide 1 by that; subtract the quotient from 1; multiply by 3. Widen to binary64 and raise binary64 2 to that power through the power import; narrow the result to binary32. Separately convert the integer to binary32, multiply by 0.25, widen to binary64, raise binary64 2 to that power through the power import, and narrow to binary32. The result is the binary32 product of the two narrowed values. The bit-exact behaviour of the power import is a property of the runtime library and is not established here.

## 12. Unresolved and limits

- The game meaning of the actor fields, flag bits, kind numbers, item categories, subtypes and the `+0x0C` / `+0x0D` bytes is not asserted; only the numeric tests and writes are established.
- The bit-exact results of the runtime power import, and therefore the exact amount in section 10.1 for inputs where that matters, are unresolved.
- The contents and population of the world list used in section 11.5 are unresolved.
- The nested world-drop helper was not re-audited; its draws and effects are taken as a separate contract.
- The storage-growth callees of the two list-append routines were not audited.
- Item bytes `+0x02`, `+0x03`, `+0x0F`, `+0x12`, `+0x13` in produced items are not established.
- Faulting or degenerate integer cases (empty world list, zero or negative level span, nonpositive or unrepresentable amount) are stated as they stand in the instructions without reachability analysis.
- Floating-point control state, exceptions and non-finite inputs are not addressed.
- Concurrency, the lifetime of the actor and its item records during the helper, and whether dropped worn items are later removed from the actor by other code are not addressed.
- Which actors and kinds actually reach this helper, and everything upstream of its entry, are outside this contract.
- No total draw count for a fatal actor is given beyond the per-branch counts above, because the nested draws and the inventory-unit branch are specified elsewhere.
