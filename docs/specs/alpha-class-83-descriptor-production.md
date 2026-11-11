Status: FINAL / STABLE / HELD

# Original production of class-0x83 descriptors and their stock dispatch

The supported originals are the 32-bit x86 images `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, both with preferred base `0x00400000`. Both hashes were recomputed before use and matched. Addresses are RVAs in the named image. Evidence paths are relative to the original evidence root, `Documents/disassembled alpha cube world`. Every statement below was read from the instruction listings (`decompiled/client/assembly.txt`, `decompiled/client/assembly-functions/005af170.asm`, `decompiled/server/assembly.txt`) and, where the listing omits bytes, from the image bytes themselves. The three earlier stock contracts were used only to locate routines; nothing is taken from them as evidence. The separately permitted reconstruction reference was not consulted, so this contract contains no reconstruction-derived statement. Client and server findings were each established in their own image; where a point was checked in only one image this is said.

This is a bounded contract about four questions: what selects a stock body, where class byte `0x83` is written, whether the two images agree, and what the stock body receives and when. It is not a contract for settlement generation, template stamping or actor creation.

## 1. There is no class-value dispatcher; the stock body is chosen by a plot-kind discriminator

No routine was found, in either image, that reads a descriptor or creature class byte and then selects a stock-generation body. Each of the three stock bodies has only direct call sites, and every one of them lies inside the single large settlement routine (client `0x1AF170`, server `0x0E28E0`). The listing contains exactly four such calls per image and no instruction that pushes or loads the address of a stock body as an immediate. Function-pointer references stored in data sections were not searched; an indirect caller through data is therefore not excluded, only not observed. [Evidence: `decompiled/client/assembly.txt`, all direct calls to VA `005c9b30`, `005c8390`, `005ca0a0`; `decompiled/server/assembly.txt`, all direct calls to VA `004fd920`, `004fc180`, `004fde90`.]

The decision is made on a 32-bit discriminator at `+0x60` of a pointed object taken from the first supplied object's pointer collection at `+0x88`. The class byte is not tested; it is written as a constant in the same branch that makes the stock call. The branch is reached only when the second supplied object's 32-bit `+0x18` equals 1, and only for a pointed object whose collection at `+0x24` is nonempty. The discriminator is compared for equality in this fixed order: 1, 2, 4, 3, 5; anything else falls to the final branch. Each of the five equality branches additionally requires the pointed object's collection at `+0x3C` to be nonempty and otherwise does nothing for that pointed object.

| Order tested | Discriminator `+0x60` | Class byte written at descriptor `+0x30` | Stock body called | Client compare / class write / call | Server compare / class write / call |
| --- | --- | --- | --- | --- | --- |
| 1st | 1 | `0x84` | none | `0x1BCBCA` / `0x1BCC7A` / — | `0x0F033A` / `0x0F03EA` / — |
| 2nd | 2 | `0x80` | client `0x1C9B30`, server `0x0FD920` | `0x1BCD8E` / `0x1BCE42` / `0x1BCE53` | `0x0F04FE` / `0x0F05B2` / `0x0F05C3` |
| 3rd | 4 | `0x82` | client `0x1C8390`, server `0x0FC180` | `0x1BCF6A` / `0x1BD01E` / `0x1BD02F` | `0x0F06DA` / `0x0F078E` / `0x0F079F` |
| 4th | 3 | `0x81` | client `0x1CA0A0`, server `0x0FDE90` | `0x1BD146` / `0x1BD1FA` / `0x1BD20B` | `0x0F08B6` / `0x0F096A` / `0x0F097B` |
| 5th | 5 | `0x83` | client `0x1CA0A0`, server `0x0FDE90` | `0x1BD322` / `0x1BD3D6` / `0x1BD3E7` | `0x0F0A92` / `0x0F0B46` / `0x0F0B57` |
| otherwise | any other value | `0x88` (different descriptor loop) | none | from `0x1BD531` / `0x1BD61E` | from `0x0F0CA1` / `0x0F0D8E` |

The decisive finding for class `0x83`: the discriminator-5 branch calls the very same stock body as the discriminator-3 branch. In the client the call instruction at `0x1BD3E7` has a relative target that resolves to `0x1CA0A0`; in the server the call at `0x0F0B57` resolves to `0x0FDE90`. These targets were recomputed from the call-instruction bytes in each image. Class `0x83` produced on this route therefore receives the stock described by the existing contracts for the body reached by class `0x81`; no separate `0x83` stock body exists on this route. That body does not read the class byte, so nothing inside it distinguishes the two classes. [Evidence: `binary/Cube.exe`, bytes at RVA `0x1BD3D6–0x1BD3EB`; `binary/Server.exe`, bytes at RVA `0x0F0B46–0x0F0B5B`; `decompiled/client/assembly-functions/005af170.asm`, VA `005bcbc5–005bd531`; `decompiled/server/assembly.txt`, VA `004f0335–004f0ca1`.]

The discriminator-1 branch (`0x84`) and the fall-through branch (`0x88`) make no stock call. Their remaining behaviour is outside this contract.

## 2. Producer A: the settlement traversal branch (stock-bearing)

This is the discriminator-5 branch of the table above. Its ordered operations, identical in both images, are:

1. A block of `0x10F0` bytes is allocated and the descriptor constructor runs (client `0x1ADB20`, server `0x0E0F40`).
2. The element count of the pointed object's `+0x3C` collection is obtained; one random draw follows; the draw is divided as an unsigned 32-bit value by that count and the unsigned remainder indexes the collection. The selected element is handed to the position service (client `0x1A6A70`, server `0x0D99D0`) with descriptor `+0x10` as destination.
3. Descriptor `+0x28` receives the 32-bit value 3.
4. A second random draw is reduced by signed remainder modulo 2 (sign-preserving, so 0 or 1 for nonnegative draws, and it may be −1 for a negative draw) and 2 is added; the 32-bit result is stored at descriptor `+0x2C`.
5. Descriptor `+0x34` receives the second supplied object's 32-bit `+0x24`.
6. Descriptor byte `+0x30` receives `0x83`. Byte `+0x31` is not written here.
7. The stock body is called.
8. A behaviour object is assembled from several small allocations and stored at descriptor `+0x109C`.
9. The descriptor pointer is appended to the first supplied object's pointer collection at `+0x18` (client call at `0x1BD527`, server call at `0x0F0C97`).

Exactly two explicit random draws occur between construction and the stock call, in the order position then kind. No draw separates the class write from the stock call. Draws made inside the constructor, the position service or the behaviour-object helpers were not audited. [Evidence: `decompiled/client/assembly-functions/005af170.asm`, VA `005bd32b–005bd52c`; `decompiled/server/assembly.txt`, VA `004f0a9b–004f0c9c`.]

### What the stock body is given

The call pushes, last to first, the descriptor pointer as the first stack argument and two 32-bit scalars as the second and third. The enclosing routine's saved receiver is loaded into ECX shortly before the call. The two scalars come from two locals that are set to 1 and 20 immediately before the pointed-object traversal begins; a helper is then offered the addresses of both locals when a preceding lookup returns a non-null object, so either may be changed before any branch runs (client: initialised at `0x1BCB06` and `0x1BCB10`, lookup `0x07FAA0`, helper `0x1EE4B0` called at `0x1BCB5D`; server: initialised at `0x0F0276` and `0x0F0280`, lookup `0x02E220`, helper `0x122290` called at `0x0F02CD`). The same two locals are passed unchanged to every stock-bearing branch for every pointed object of that traversal.

Inside the stock body reached for class `0x83`, a scan of every instruction of the body found a single read of an incoming stack argument: the first one, the descriptor. The second and third arguments are never read, and the body returns releasing three stack arguments. The only descriptor member it addresses is the inventory at `+0xF6C`. It does not read descriptor `+0x28`, `+0x2C`, `+0x30`, `+0x31` or `+0x34`. Consequently the descriptor level written in step 5 and the two scalar locals do not influence the generated stock, and no seed is passed: the body draws from the process-wide random import in call order (client import slot RVA `0x2FC468`, server `0x1582F4`). [Evidence: `decompiled/client/assembly.txt`, function at VA `005ca0a0`, argument read at `005ca0ce`, inventory offset at `005cad22`, return at `005cadd2`; `decompiled/server/assembly.txt`, function at VA `004fde90`, argument read at `004fdebe`, inventory offset at `004feb12`, return at `004febc2`.]

### Lifecycle position

Stock is generated exactly once on this route, on a freshly constructed descriptor, after the class byte is written and before the behaviour object exists and before the descriptor pointer is published into the first supplied object's `+0x18` collection. No later regeneration, refresh or replacement was sought or found within this bounded audit.

## 3. Where the discriminator value 5 comes from

The pointed object's `+0x60` is written once, when the pointed object is created, as a copy of the 32-bit field at `+0x10` of a 28-byte plot record (seven 32-bit words; the record array base and the record index are locals of the settlement routine). Client: copy at `0x1B2DED–0x1B2DF7`; server: `0x0E655D–0x0E6567`. Immediately afterwards record values 2, 3, 4 and 5 share one continuation and value 1 takes another.

The record field `+0x10` is assigned in only two places per image:

- the value 1, to the record whose index is removed from the end of a pending index list, when the first supplied object's byte `+0x79` is 0 or 3 and the list holds more than one entry (client `0x1AFDA7–0x1AFDDE`, server `0x0E3517–0x0E354E`);
- the values 2, 3, 4, 5 in that order, when the first supplied object's byte `+0x79` equals 1: a counter runs 0, 1, 2, 3; on each pass, if the pending list is empty the loop ends early, otherwise the last index is removed from the list and that record's `+0x10` receives counter plus 2 (client `0x1AFDE6–0x1AFE36`, server `0x0E3556–0x0E35A6`).

Both assignments are inside the part of the routine guarded by the second supplied object's `+0x18` being 1. For byte `+0x79` equal to 1 there is one earlier removal on this path: if the list initially holds more than one entry, its last index is removed and that record's state word `+0x0C` becomes 9 (client `0x1AFA71–0x1AFA99`, server `0x0E31E1–0x0E3209`). The intervening blocks are conditioned on byte `+0x79` being 3 or 2 and are skipped, so no random draw occurs between that removal and the four kind assignments.

Therefore the kind value 5, and with it class `0x83`, exists only for a first supplied object whose byte `+0x79` is 1, and goes to the fourth index taken from the end of the pending list after the state-9 removal; it requires that at least four entries remain at that point. The pending list is built in ascending record order from records whose state word `+0x0C` equals 2 and is then passed to a helper (client `0x1AE540`; the server makes a call at the corresponding point, immediately before `0x0E31CC`) whose effect on the list's order was not audited. Which physical plot receives kind 5 is therefore unresolved here, as are the producers of the plot records, of byte `+0x79` and of the pointed object's `+0x3C` position collection. [Evidence: `decompiled/client/assembly-functions/005af170.asm`, VA `005af9a0–005afe3e`, `005b2ded–005b2e2b`; `decompiled/server/assembly.txt`, VA `004e31cc–004e35ae`, `004e655d–004e659b`.]

## 4. Producer B: the template-stamping routine (no stock)

Class byte `0x83` is written at exactly one other instruction in each image, inside a template-stamping routine (client `0x1F0CE0`, write at `0x1F42B9`; server `0x124540`, write at `0x127B19`). A search of both full listings for a one-byte immediate store of `0x83` to offset `+0x30` of any register found these two sites per image and no others. Stores of a class value held in a register, and copies of whole descriptors, are not covered by that search.

The routine takes its receiver in ECX and nine 32-bit stack arguments (it releases `0x24` bytes). It walks the cells of a three-dimensional template whose three extents are at `+0x44`, `+0x48`, `+0x4C` of the first argument and whose cells are three-byte triples at the address held in `+0x30`. The third argument is an orientation, reduced by signed remainder modulo 4; the fifth argument is a 32-bit type selector; the sixth argument is an object whose pointer collection at `+0x18` receives produced descriptors.

A descriptor is produced for a cell when all of the following hold (client-verified; server-verified from the marker comparison onward):

- the selector is one of 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15 (client `0x1F169F–0x1F16E6`); other selector values bypass the marker chain;
- the cell's triple failed every earlier marker comparison of the chain; the five immediately preceding comparisons are against (255, 0, 255), (0, 255, 255), (0, 0, 127), (0, 255, 127) and (0, 127, 0), in that order, each triple given in stored byte order 0, 1, 2 (client compares at `0x1F326F`, `0x1F33E5`, `0x1F39F2`, `0x1F3B68`, `0x1F3FBF`);
- the cell's triple equals (127, 0, 127) byte for byte (client compare at `0x1F4182`, server `0x1279E2`).

The head of the marker chain and the tests applied to the destination world cell before the chain were not closed.

On a match the ordered operations are: allocate `0x10F0` bytes and construct a descriptor; store 32-bit 3 at `+0x28`; make one random draw, reduce it by signed remainder modulo 4 and store the 32-bit result at `+0x2C` with nothing added; compute 2 minus the orientation argument as a signed 32-bit integer, multiply by 90, convert to binary32 and store at `+0x54`; write the position at `+0x10` through vector helpers using the half-unit constants `0.5` (bit pattern `0x3F000000`) three times; store the 16-bit value 4 at `+0x350` and the byte 6 at `+0x35D`; obtain a level from the level service described below and store it as 32 bits at `+0x34`; then switch on the selector.

| Selector (fifth argument) | Effect at the switch |
| --- | --- |
| 6 | byte `+0x30` becomes `0x83` |
| 7 | `+0x34` overwritten with 20; one further draw, signed remainder modulo 2 plus 2, stored at `+0x2C`; byte `+0x30` becomes `0x89` |
| 8 | nothing |
| 9, 10, 11, 12 | four longer blocks; the block for 9 writes class byte 1 among other fields; the others were not read |
| 13, 14, 15 | byte `+0x30` becomes `0x85`, `0x86`, `0x87` respectively |
| below 6 or above 15 | nothing |

The switch is an unsigned range test on selector minus 6 against 9 followed by a ten-entry address table; the table was decoded from the image bytes in both images (client table at RVA `0x1F4B20`, server `0x128380`). After the switch every case continues to the same tail: a behaviour object is assembled and stored at `+0x109C`, and the descriptor pointer is appended to the sixth argument's `+0x18` collection (client call at `0x1F4886`, server `0x1280E6`).

For selector 6 there is exactly one explicit random draw in the whole production sequence, the modulo-4 draw, and it precedes the class write. No stock body is called anywhere in this routine. A class-`0x83` descriptor produced here has only the constructor's empty inventory at the moment of publication. Whether anything fills that inventory later is unresolved; what is established is that none of the three audited stock bodies can be the one to do so, because they have no other caller. [Evidence: `decompiled/client/assembly.txt`, function at VA `005f0ce0`, especially `005f416a–005f42bd` and `005f4735–005f488b`; `binary/Cube.exe`, bytes at RVA `0x1F4B20`; `decompiled/server/assembly.txt`, function at VA `00524540`, especially `005279ca–00527b1d` and `00527f95–005280eb`; `binary/Server.exe`, bytes at RVA `0x128380`.]

### Level service used by producer B

The level service (client `0x19A9B0`, server `0x0D2340`) receives two 64-bit coordinates taken from the first two components of the cell's world position. It derives one 32-bit lookup index from each. For the first coordinate: signed 64-bit division by 65536, then the low 32 bits divided as a signed integer by 2048 with rounding toward zero. For the second coordinate: conversion of the signed 64-bit value to binary64, multiplication by the binary64 constant 1/2048 (bit pattern `0x3F40000000000000` in both images), conversion back to a 64-bit integer through a runtime conversion helper that was not audited, then signed 64-bit division by 65536. With those two indices it looks up a region object, and returns that object's 32-bit `+0x24` when the object exists, its 32-bit `+0x18` is nonzero, and a distance-like binary32 measure returned by a further helper is not greater than binary32 1 (bit pattern `0x3F800000`). In every other case it returns 1. The lookup and the distance helper were not audited. This is a different level source from producer A, which copies the second supplied object's `+0x24` directly.

### Which callers supply selector 6

The stamping routine has 37 direct call sites in each image. The fifth argument was identified at every client site by counting pushes back from the call:

- four sites inside the settlement routine pass a local that holds a mapping of the current plot record's `+0x10` kind (client calls at `0x1B4BD7`, `0x1B51B0`, `0x1B5788`, `0x1B5D5B`; server `0x0E8347`, `0x0E8920`, `0x0E8EF8`, `0x0E94CB`);
- one site passes a second local produced by a different six-way mapping whose results are 9 through 15 (client-decoded only);
- every other site passes a constant, and none of those constants is 6.

The mapping local is set to 1 and then replaced according to the record kind: kind 1 gives 2, kind 2 gives 3, kind 3 gives 4, kind 4 gives 5, kind 5 gives 6, and any other kind leaves 1. The listing does not disassemble the five target blocks; they were decoded from the image bytes in both images and agree (client table RVA `0x1BF404`, blocks at `0x1B4566–0x1B459F`; server table RVA `0x0F2B74`, blocks at `0x0E7CD6–0x0E7D0F`).

So selector 6, and hence producer B's class `0x83`, arises only for a plot record of kind 5 — the same record kind that yields discriminator 5 for producer A. Both producers are consequences of one upstream selection. Whether the templates passed at those four sites actually contain a (127, 0, 127) cell is a property of template data and is unresolved; if they do not, producer B never fires for selector 6. The relative order of the four stamping calls and the pointed-object traversal for one plot, and the number of descriptors of each kind that result, were not closed.

## 5. Fields of a class-0x83 descriptor at publication

| Descriptor member | Width | Producer A (traversal branch) | Producer B (template stamping) |
| --- | --- | --- | --- |
| `+0x10` | position storage | from a randomly indexed element of the pointed object's `+0x3C` collection | from the matched cell's world position with half-unit offsets |
| `+0x28` | 32 bits | 3 | 3 |
| `+0x2C` | 32 bits | signed remainder modulo 2 of a draw, plus 2 | signed remainder modulo 4 of a draw, nothing added |
| `+0x30` | 8 bits | `0x83` | `0x83` |
| `+0x31` | 8 bits | not written after construction | not written after construction |
| `+0x34` | 32 bits | second supplied object's `+0x24` | level-service result, or 1 |
| `+0x54` | binary32 | not written | (2 − orientation) × 90, converted from signed 32-bit |
| `+0x350` | 16 bits | not written | 4 |
| `+0x35D` | 8 bits | not written | 6 |
| `+0x109C` | pointer | behaviour object | behaviour object |
| inventory `+0xF6C` | — | filled by the stock body before publication | left as constructed |

"Not written" means not written by the producing branch after the constructor returns; the constructor's own initial values for those members were not re-audited here.

## 6. Client and server agreement

Each point below was read independently in each image and found to have the same structure, constants, operand widths and ordering:

- the order of discriminator comparisons, the class constant of every branch, and which branches call a stock body;
- that the discriminator-5 branch calls the same body as the discriminator-3 branch;
- the two explicit draws of producer A, their reductions, and the order of the field writes, stock call, behaviour assembly and publication;
- the initial values 1 and 20 of the two unread scalar arguments and the optional helper that may alter them;
- that the reached stock body reads only its first stack argument and addresses only inventory `+0xF6C`;
- the copy of the plot-record kind into the pointed object's discriminator, and the two assignment sites of the record kind including the four-step loop that yields 2, 3, 4, 5;
- producer B's marker triple, its single modulo-4 draw, its field writes, its ten-entry selector switch, and its publication tail;
- the level service, including the comparison constant and the fallback value 1;
- the kind-to-selector mapping and the four call sites that pass it.

Verified in the client only: the admitted-selector list ahead of the marker chain, the full per-call-site census of the fifth argument, and the decoding of the second mapping that yields 9 through 15. The server listing shows the same push patterns at its 37 call sites, but the server's second mapping table was not decoded from bytes. No behavioural difference between the images was found on any point examined.

## 7. Unresolved

- Whether any data-section function pointer refers to a stock body (only direct calls and immediate operands were searched).
- Whether class `0x83` is ever written from a register or by copying another descriptor, outside the two immediate-store sites.
- Whether producer B's templates for selector 6 contain the marker triple, and therefore whether stock-less class-`0x83` descriptors are produced in practice.
- The head of producer B's marker chain, the destination-cell tests before it, and the selector blocks for 9 through 12.
- The ordering helper applied to the pending plot list, and thus which plot receives kind 5; the producers of the plot records, of the first supplied object's byte `+0x79`, and of the pointed object's `+0x3C` collection.
- The source of the two unread scalar arguments beyond their initial values, and the effect of the helper that may alter them.
- Draws and side effects inside the descriptor constructor, the position service, the level-service helpers and the behaviour-object helpers.
- The relative order of producers A and B for one plot, descriptor counts, later actor creation, and any later stock refresh or inventory population for either producer.
- Any meaning or display name of class `0x83`, of the plot kinds, or of the selector values; none is asserted here.
