Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha airship companion: its behaviour object and element, and the zone generator's finishing step for its spawn record

## 1. Scope and evidence

This contract specifies (a) the behaviour object attached to the airship companion spawn record and its single element, as they act on the companion creature during the world update, and (b) the zone generator's finishing step as it is applied to that record (record kind 4). The held contract `alpha-airship-steering.md` was read only to locate the record, the behaviour constructor and the spawn pass, and for terms; every statement below was re-derived from the original images.

Supported images, both 32-bit with preferred base `0x00400000`; hashes were recomputed before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs (virtual address minus `0x00400000`), client unless marked "server". Evidence: the instruction listings `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, the call lists `decompiled/<image>/calls.csv`, and the image bytes (read through each image's section table) for every virtual table, run-time type record and constant. Decompiler pseudocode was not relied on. Where the client listing has small undecoded gaps (at `0x0DC377` and `0x02EF98`), the missing instructions were read from the image bytes.

The isolated Infinite-Cubes alpha reconstruction was consulted for leads only. Every statement resting on it is marked "(reconstruction lead)"; such leads concern only the meaning of fields, never their offsets, widths, values or order, which come from the original.

### Terms

- **F(v)**, per component of a signed 64-bit triplet difference: the value loaded by the x87 unit, stored to binary32 under the ambient x87 rounding control, then multiplied in binary32 by 2⁻¹⁶ (binary32 `0x37800000`; client image `0x2FCD94`, server `0x15869C`).
- **Q(d)**: ((d.y × d.y) + (d.x × d.x)) + (d.z × d.z), each product and sum rounded to binary32. (Binary32 addition being commutative, this equals the squared length with the x and y terms in either order.)
- **C**: the creature whose behaviour is being run. **W**: the world.
- **Creature map**: the ordered map at world `+0x04`, keyed by creature identifier, whose node value at node `+0x18` is the creature pointer (confirmed by the creature lookup `0x02F000`, which searches world `+0x04` and returns node `+0x18`, or null at the end). It is walked by in-order successor, i.e. in ascending key order (the key comparison's signedness was not verified).

## 2. Identification of the two classes

### 2.1 Virtual tables and run-time type names

Both images carry run-time type information. The complete-object locator precedes each virtual table; its type record names the class and its hierarchy lists exactly two classes (the class itself and one base).

| Role | Client vtable | Server vtable | Run-time type name | Base | Slots (client; server) |
|---|---|---|---|---|---|
| Behaviour object (record `+0x109C`, creature `+0x13E4`) | `0x303A00` | `0x15B028` | `.?AVSequentialBehavior@cube@@` | `.?AVBehavior@cube@@` | 0 `0x0DC390`, 1 `0x0DC400`, 2 `0x0DC360`; server `0x01D080`, `0x01D0F0`, `0x01D050` |
| Its list element | `0x302D60` | `0x15AC70` | `.?AVLookAtPlayerBehavior@cube@@` | `.?AVBehavior@cube@@` | 0 `0x0C8140`, 1 `0x0C83D0`; server `0x014730`, `0x0149C0` |

Locators: client `0x3478BC` (behaviour object) and `0x3474C0` (element); server `0x174280` and `0x17410C`. The table size was taken as the run of code pointers before the first non-code word: three for the behaviour object, two for the element.

Slot meanings, from the instructions:

- Slot 0 of both classes is the **update** method: four stack arguments (creature, world, a millisecond count, a fourth value), removed by the callee (16 bytes), returning a byte used as a Boolean.
- Slot 1 of both classes is the **clone** method: no arguments, returns a newly allocated copy (null when the allocation returns null).
- Slot 2 of the behaviour object only is the **deleting destructor** (one argument; bit 0 selects freeing). The element class has no virtual destructor slot.

### 2.2 Object layouts

- **Behaviour object** (12 bytes; constructor `0x0DC2D0`, server equivalent not separately located): `+0x00` vtable; `+0x04` pointer to the sentinel node of a circular doubly linked list (allocated empty by `0x06D550`, the sentinel linking to itself); `+0x08` 32-bit element count. A list node is 12 bytes: `+0x00` next, `+0x04` previous, `+0x08` element pointer. (The append helper `0x0860B0` refuses growth beyond `0x15555554` entries, consistent with 12-byte nodes; the update reads the element at node `+0x08`.)
- **Element** (4 bytes; constructor `0x0C8130`, which only stores the vtable): no state beyond its vtable pointer. All instances are interchangeable.

### 2.3 Construction for the companion

At client `0x1BAA87`–`0x1BAB0A` the layout routine allocates the 12-byte behaviour object and constructs it with `0x0DC2D0`, allocates a 4-byte element and constructs it with `0x0C8130`, appends that element at the end of the list (`0x0860B0`), and stores the behaviour object into record `+0x109C`. Exactly one element is appended. If either allocation returns null, a null pointer is stored or appended instead (not further specified).

### 2.4 Copy to the creature (slot 1, `0x0DC400`; server `0x01D0F0`)

The spawn pass stores into creature `+0x13E4` the result of the record behaviour's slot 1. That method allocates 12 bytes (null result if the allocation returns null) and fills them by `0x0DC200` (server `0x01CEF0`): vtable `0x303A00`, a new empty list, count 0; then for each source element in list order it calls that element's slot 1 and appends the returned pointer at the end. The element's slot 1 (`0x0C83D0`) allocates 4 bytes and stores the element vtable, returning null when the allocation fails. The creature therefore holds a fresh behaviour object with one fresh element of the same class. No draw occurs.

### 2.5 Destruction (observable only as memory behaviour)

The behaviour object's destructor (`0x0DC300`, reached from slot 2) frees each element's storage with the plain allocator release, without calling any element method, then frees the nodes and the sentinel. The creature's own destructor (`0x03C410`) releases creature `+0x13E4` with the plain release, without calling slot 2, so the list and element of a creature's behaviour are not released there. Neither path affects behaviour.

## 3. When the behaviour runs (world update, client `0x211A96`–`0x211B75`; server `0x137856`–`0x137935`)

Inside the per-creature body of the world update `0x20C510` (server `0x1322D0`), after a clamp (`0x211A96`–`0x211ADB`) of creature `+0x16C` (binary32) to an upper value supplied by `0x044DB0` (server `0x00FDA0`; not audited: when `+0x16C` is greater, ordered strict, it is replaced by a second call's result) and only when 0.0 ≥ `+0x16C` is false (an ordered test; a NaN passes), the following happens, in order:

1. If creature byte `+0x60` is 0, or world byte `+0xB4` is not 0, the whole block is skipped (the creature's `+0x13C4` is not touched).
2. Creature `+0x13C4` (signed 32-bit) is increased by the world update's first argument, the step in milliseconds (wrapping addition).
3. If creature `+0x12C` (signed 32-bit) is greater than 0, the behaviour is not run and `+0x13C4` is not reset; the routine `0x02F160` is then called on creature `+0x10` with the arguments `0x80` and 0 (not audited).
4. Otherwise, if `+0x13C4` is less than 32 (signed), nothing more happens in this block.
5. Otherwise, if creature `+0x13E4` is not null, its slot 0 is called with (creature, world, the accumulated `+0x13C4` value, the world update's second argument). Whether or not it was null, `+0x13C4` then becomes 0. If, after the call, `+0x12C` is greater than 0, the call of step 3 is made.

The Boolean returned by the behaviour is discarded by this caller. The companion has byte `+0x60` = 3 (copied from record `+0x28`), so it passes step 1 whenever world byte `+0xB4` is 0. The behaviour therefore runs at most once per world update, and only on updates in which at least 32 milliseconds have accumulated since its last run. The third and fourth arguments are passed through by the behaviour object and are not read by the element class.

(Reconstruction leads: `+0x60` is the creature's hostility class with 0 meaning a player; `+0x16C` is health.)

## 4. Behaviour object update (slot 0, `0x0DC390`; server `0x01D080`)

1. The list is walked from the first node in list order. For each node, the element at node `+0x08` has its slot 0 called with the same four arguments the behaviour received.
2. At the first element whose call returns a non-zero byte, the walk stops and the method returns true. Later elements are not called.
3. If the list is empty or every element returned zero: the three 32-bit words at creature `+0x40`, `+0x44` and `+0x48` are set to 0 (binary32 +0.0) and the method returns false.

There is no other read or write and no draw.

## 5. Element update (slot 0, `0x0C8140`; server `0x014730`)

Inputs: creature C (first argument), world W (second). The third and fourth arguments are not read.

### 5.1 Nearest qualifying player

1. Best distance b starts at 64.0 (binary32 `0x42800000`; client image `0x2FD330`, server `0x15882C`); the candidate P starts as none.
2. Every entry of the creature map is visited in ascending key order. An entry qualifies only when, in this order: its creature pointer X is not null; X byte `+0x60` is 0; and X `+0x16C` ≥ 0.0 as an ordered comparison (a NaN or a negative value disqualifies; exactly 0.0 qualifies).
3. For a qualifying X: d = F(X position `+0x10` − C position `+0x10`), the subtraction 64-bit and wrapping per component; q = Q(d). If b > q (ordered strict; a NaN q is never taken), P becomes X and b becomes q.

Hence P is the first, in key order, of the qualifying creatures at the smallest squared distance strictly below 64.0 (that is, within 8 world units). C itself never qualifies, since the behaviour is only run for creatures whose byte `+0x60` is not 0.

### 5.2 Decision and writes

4. Bit `0x0004` of the 16-bit field at creature `+0x124` is cleared, unconditionally.
5. If there is no P, the method returns false. Nothing else is written.
6. If C byte `+0x68` is `0x53` (83) or `0x54` (84), the method returns false. Nothing else is written (the bit stays cleared).
7. Otherwise:
   - bit `0x0004` of creature `+0x124` is set;
   - the binary32 triplet at creature `+0x160`, `+0x164`, `+0x168` becomes F(P position − C position), recomputed in the same way as in 5.1 (same values as the d that selected P);
   - the three words at creature `+0x40`, `+0x44`, `+0x48` become 0 (binary32 +0.0);
   - the routine `0x02EF10` (server `0x005330`) is called on C: it empties the containers at creature `+0x140C` and `+0x1414` (ordered trees, emptied by `0x02CA20`), empties the list at `+0x1460` and sets its count `+0x1464` to 0, sets `+0x1408` to the sentinel of `+0x140C`, and empties the ordered tree at `+0x1468` (`0x27E480`); it reads nothing else and makes no draw;
   - the method returns true.

No random draw occurs anywhere in the element (checked by the call list: its only callees are `0x02EF10`, the tree-emptying helpers and the stack-check routine).

(Reconstruction leads, not confirmed: `+0x40` is the creature's commanded acceleration, i.e. its movement input; `+0x160` is its aim or facing offset; bit `0x0004` of `+0x124` makes the creature face that offset; byte `+0x68` is its current action, with 83 and 84 being two resting actions; the containers emptied by `0x02EF10` hold its path-finding state.)

### 5.3 Net effect for the companion

The companion's behaviour has this element as its only element. On every run (Section 3):

- with a qualifying player within 8 world units and the action byte not 83 or 84: the facing bit is set, the aim triplet points at the nearest such player (in world units, unnormalised), the movement input is zeroed, the path state is emptied, and the behaviour returns true;
- otherwise: the facing bit is cleared, the movement input is zeroed by the behaviour object (Section 4 step 3), and the behaviour returns false.

In both cases the movement input `+0x40`..`+0x48` is +0.0 after a run. The behaviour never writes the creature's position, velocity, yaw or any target identifier, and makes no draw.

## 6. Comparison with the other behaviour classes

The client image carries eight classes derived from the behaviour base (run-time names below; the base itself has no virtual table with a locator):

| Run-time type name | Client vtable | Server vtable | Slot 0 / slot 1 (client) | Slot 0 / slot 1 (server) | Client constructor(s) |
|---|---|---|---|---|---|
| `.?AVCombatBehavior@cube@@` | `0x2FD310` | `0x1587E0` | `0x02CB20` / `0x02EFC0` | `0x002F40` / `0x0053E0` | `0x02C550` (one binary32 argument) |
| `.?AVCompanionBehavior@cube@@` | `0x2FD868` | `0x158850` | `0x03ACE0` / `0x03B450` | `0x005780` / `0x005EF0` | `0x03AB80`, `0x03ABA0` |
| `.?AVLookAtPlayerBehavior@cube@@` | `0x302D60` | `0x15AC70` | `0x0C8140` / `0x0C83D0` | `0x014730` / `0x0149C0` | `0x0C8130` |
| `.?AVRandomInteractionBehavior@cube@@` | `0x3039D0` | `0x15AFF0` | `0x0DB130` / `0x0DBEF0` | `0x01BC20` / `0x01CAC0` | `0x0DB100` |
| `.?AVRandomWalkBehavior@cube@@` | `0x3039E8` | `0x15B010` | `0x0DBF20` / `0x0DC1E0` | `0x01CBB0` / `0x01CE70` | `0x0DBF10` |
| `.?AVSequentialBehavior@cube@@` | `0x303A00` | `0x15B028` | `0x0DC390` / `0x0DC400` (slot 2 `0x0DC360`) | `0x01D080` / `0x01D0F0` (slot 2 `0x01D050`) | `0x0DC2D0` |
| `.?AVSpawnLocationBehavior@cube@@` | `0x303B58` | `0x15B250` | `0x0DFAD0` / `0x0E0050` | `0x028940` / `0x028EC0` | `0x0DFAB0` |
| `.?AVWalkPathBehavior@cube@@` | `0x31A770` | `0x1716A0` | `0x18D050` / `0x18D630` | `0x0C5E00` / `0x0C63E0` | `0x18CF60`, `0x18CFA0` (one binary32 argument) |

Findings for the implementer:

- The companion's element **is** the ordinary look-at-player element: same class, same virtual table, same stateless 4-byte object, same constructor `0x0C8130`. At every construction site listed below, a sequential object is constructed first and the element constructors follow; the companion's behaviour object differs from those only in its element list.
- The companion record does **not** use the class named "CompanionBehavior"; that is a different class with its own methods (not audited here).
- Ordinary record lists observed in the layout routine `0x1AF170`: at `0x1BCC96`–`0x1BCD89` the list is built as a combat element (constructed with binary32 20.0, `0x41A00000`, appended first), then a look-at-player element (appended second), then a walk-path element (constructed with binary32 2.0, `0x40000000`, and then passed a position triplet by `0x1BF4F0`, not audited; its append happens at a shared tail at `0x1BD4F9` that was not audited). The call list shows, for the other sequential constructions of that routine, constructor-call orders combat / look-at-player / walk-path (sites `0x1BCE72`, `0x1BD04E`, `0x1BD22A`, `0x1BD406`), combat / look-at-player / spawn-location / random-interaction / random-walk (`0x1BDD6D`), and combat / walk-path (`0x1BE6A9`, `0x1BEDC2`, `0x1BF109`); for those sites the append order was not individually verified. Constructor calls of these classes also occur in `0x1CC510`, `0x1D6040`, `0x1D9460`, `0x1DC6E0`, `0x1EE7A0`, `0x1F0CE0` and in the world update; they were not audited.
- Consequence of the order: in a list with a combat element ahead of the look-at-player element, the look-at-player element runs only when the combat element returned zero. The companion has no such predecessor, so its element runs on every behaviour run.
- The other classes' update methods were not audited; this contract makes no statement about their decisions or draws.

## 7. The finishing step applied to the companion record (client `0x03F7C0`, server `0x00A840`)

### 7.1 Call

In the zone generator `0x1E4850` (server `0x118630`), for each spawn record in list order, after the identifier assignment, the step is called at client `0x1ED32F` (server `0x12110F`) with three stack arguments: a pointer to record `+0x2C` (the record kind, 32-bit), a pointer to the appearance block at record `+0x74`, and 0. The caller removes the arguments. For the companion the kind is 4.

Below, appearance offsets are relative to record `+0x74`; the record offset is given in brackets.

State before the step, for the companion: the appearance block was constructed by `0x028750` inside the record constructor `0x1ADB20`, giving bytes `+0x00` = 0, `+0x01` = 0, `+0x02`..`+0x04` = 255 each; 16-bit `+0x06` [record `+0x7A`] = 0; binary32 `+0x08`, `+0x0C`, `+0x10` = 1.0; the eight 16-bit fields `+0x14`..`+0x22` = `0xFFFF` each; binary32 `+0x38` = binary32 `0x3F733333` (about 0.95). No write to record `+0x7A` occurs in the companion block of the layout routine (`0x1BAA1B`–`0x1BAB64`); the record-flag writes found elsewhere in that routine (`0x1BE67D`, `0x1BE68D`, `0x1BE9C3`, `0x1BED7F`, `0x1BF0BD`) belong to the blocks that allocate other records (at `0x1BE591`, `0x1BE90D`, `0x1BEC28`, `0x1BF003`).

### 7.2 Draws (third argument zero)

Because the third argument is 0, the step first makes five calls of the runtime `rand` (import slot client `0x2FC468`, confirmed as the runtime random routine by the call list; server likewise). Each result is reduced by a signed 32-bit remainder; the runtime's results are non-negative, so these are ordinary remainders. In order:

1. r1 mod 256, kept as byte A.
2. r2 mod 256, kept as byte B.
3. r3 mod 256, kept as byte C.
4. r4 mod 100, kept as 32-bit D.
5. r5 mod 100, kept as 32-bit E.

These five draws are made for every record that reaches this call with a zero third argument, before the kind is examined. With a non-zero third argument a different preparation (a switch on the value it points to) replaces them; that path is not used here and was not audited.

### 7.3 Writes common to all kinds

Then binary32 `+0x08` [record `+0x7C`] and `+0x0C` [`+0x80`] become binary32 `0x3F75C290` (0.96) and `+0x10` [`+0x84`] becomes binary32 `0x400A3D71` (2.16). The kind is then read as a 32-bit value; an unsigned value above `0x9B` goes to a default path (not audited); otherwise it selects an entry of the jump table at client `0x043CA0` (server `0x00ED20`). Entry 4 is client `0x03FBF0` (server `0x00AC70`).

### 7.4 Kind 4 (client `0x03FBF0`–`0x03FC82`; server `0x00AC70`–`0x00AD02`)

In this order:

1. Byte `+0x02` [record `+0x76`] = C (the third draw); byte `+0x03` [`+0x77`] = B (second draw); byte `+0x04` [`+0x78`] = A (first draw).
2. 16-bit `+0x14` [`+0x88`] = (D mod 5) + 75. Since 100 is a multiple of 5 this equals (r4 mod 5) + 75, giving 75 to 79.
3. 16-bit `+0x16` [`+0x8A`] = (E mod 6) + 80, i.e. ((r5 mod 100) mod 6) + 80, giving 80 to 85. This is not the same as (r5 mod 6) + 80, because 100 is not a multiple of 6.
4. A 32-bit store of `0x000001B0` at `+0x1A` [`+0x8E`]: 16-bit `+0x1A` = 432 and 16-bit `+0x1C` [`+0x90`] = 0 (replacing the constructor's `0xFFFF`).
5. 16-bit `+0x18` [`+0x8C`] = 97 (`0x61`).
6. Binary32 `+0x38` [`+0xAC`] = binary32 `0x3F99999A` (1.2).
7. Binary32 `+0x08`, `+0x0C` and `+0x10`, in that order, are each divided in binary32 by binary32 `0x3F99999A` (1.2, client image `0x2FCDCC`, server `0x1586E0`), giving `0x3F4CCCCD`, `0x3F4CCCCD` and `0x3FE66666` (0.8, 0.8 and about 1.8).
8. The step returns. No other call is made on this path (apart from the stack check).

Fields not written for kind 4: bytes `+0x00` and `+0x01`; the 16-bit field `+0x06`; the 16-bit fields `+0x1E`, `+0x20`, `+0x22` (they stay `0xFFFF`); and all binary32 fields other than `+0x08`, `+0x0C`, `+0x10` and `+0x38`.

### 7.5 The two flag bits

For kind 4 the step **cannot** set bit `0x1000` or bit `0x0200` of the 16-bit field at record `+0x7A`: neither the draw preparation, nor the common writes, nor the kind-4 path writes appearance `+0x06` at all. The field keeps the value it had (0 for the companion, by 7.1).

(Reconstruction leads, not confirmed: bytes `+0x02`..`+0x04` are a hair colour; `+0x08`..`+0x10` is a body size triplet; `+0x14`, `+0x16`, `+0x18`, `+0x1A` and `+0x1C` are model indices for head, hair, hands, feet and chest; `+0x38` is a weapon scale.)

## 8. Adjacent generator steps for the same record (bounded, client `0x1ED334`–`0x1ED3D5`; server `0x121114`–`0x1211B5`)

These steps follow the finishing step in the same per-record pass. They are included only to bound draws and the flag bits for the companion.

1. The routine `0x1C7730` (server `0x0FB480`) is called with (record, 0). Its first test is bit `0x0040` of record `+0x7A`: when set, it returns at once. For the companion the bit is clear (7.5), so it next compares record byte `+0x30` against a list of values; for `0x88` (the companion's value) it goes directly to client `0x1C81EE`, where: 16-bit record `+0x350` = 4; byte `+0x35D` = 6; **one draw**: byte `+0x351` = (r mod 5) + 2 (signed 32-bit remainder, the sum taken in 8 bits); then binary32 `+0xF60` = 0.1 (`0x3DCCCCCD`), `+0xF5C` = 0.3 (`0x3E99999A`), `+0xF64` = 10.0 (`0x41200000`) and `+0xF68` = 10.0; then it returns. This routine contains no write to record `+0x7A` (only reads and tests of it); its other paths were not audited.
2. The generator then tests bit `0x0200` of record `+0x7A` (`0x1ED363`). When set it would build an item (`0x0E0B` in a 16-bit field) using one draw and add it to record `+0xF6C` (not audited further); for the companion the bit is clear, so this is skipped with no draw.
3. Further reads of bit `0x0200` (shifted down by 9) are passed to later generator calls; they were not audited.

Within these steps, therefore, both bits stay clear for the companion, and the spawn pass's special branch (which requires bit `0x1000`) and its size doubling (which requires bit `0x0200`) do not apply to the companion unless some writer outside the routines examined here changes record `+0x7A` (none was found in the companion block, the finishing step or `0x1C7730`; the generator's own writes at `0x1EB72B`, an OR of `0x0200` guarded by record `+0x28` = 1 and a draw, and `0x1EBAF0`, a clear of `0x0200`, act on records with `+0x28` = 1 or on records whose kind it reassigns, and were not traced to exclude the companion by aliasing).

## 9. Random draws (summary)

| Routine | Draws for the companion | Audit |
|---|---|---|
| Behaviour object update `0x0DC390` | none | full |
| Element update `0x0C8140` with `0x02EF10` and its helpers | none | full (helpers bounded: they only release tree and list nodes) |
| Clone `0x0DC400`, `0x0DC200`, `0x0C83D0`; constructors `0x0DC2D0`, `0x0C8130` | none | full |
| Finishing step `0x03F7C0`, zero third argument, kind 4 | 5, in the order of 7.2 | full for this path; the other kind entries, the default path and the non-zero-argument preparation were not audited |
| Follow-on `0x1C7730`, bit `0x0040` clear, byte `+0x30` = `0x88` | 1 (the sixth draw for this record in these steps, after the five above) | that path only |
| Bit-`0x0200` item step `0x1ED363` | none (bit clear) | test only |

The draws of 7.2 and of 8 item 1 are consumed from the same runtime generator in that order, interleaved with whatever the generator's per-record pass draws for earlier records. Draws in the per-creature update are absent: the companion's behaviour makes none.

## 10. Client/server agreement

Compared instruction for instruction after masking absolute addresses (values from `0x00400000` to `0x007FFFFF`), with every masked constant read from both images and found equal:

- Behaviour object update `0x0DC390` / server `0x01D080` (47 instructions), clone `0x0DC400` / `0x01D0F0` (39), copy constructor `0x0DC200` / `0x01CEF0` (69), deleting destructor `0x0DC360` / `0x01D050` (identical; the client listing omits one instruction that the image bytes contain).
- Element update `0x0C8140` / `0x014730` (188), element clone `0x0C83D0` / `0x0149C0` (9), path-state clear `0x02EF10` / `0x005330` (identical; the client listing omits a four-instruction loop tail that the image bytes contain).
- Finishing step `0x03F7C0` / `0x00A840` (3815 instructions, no differences), including its jump tables and the kind-4 case at the same offset `0x430` from the routine start.
- Follow-on `0x1C7730` / `0x0FB480` (659 instructions, no differences).
- The world update's behaviour gate and call (Section 3) at −`0xDA240`, and the generator call site (Section 7.1) and bit tests (Section 8) at −`0xCC220`: identical text.
- Constants: 64.0 (client `0x2FD330`, server `0x15882C`), 2⁻¹⁶ (`0x2FCD94`, `0x15869C`), 1.2 divisor (`0x2FCDCC`, `0x1586E0`) equal; the remaining constants are immediates in identical instructions.

No client/server difference was found in anything this contract specifies.

## 11. Unresolved

- The meaning of creature byte `+0x68` values 83 and 84, of the facing bit, of the aim triplet, of the movement-input triplet and of the containers emptied by `0x02EF10` (only reconstruction leads are given).
- The clamp source `0x044DB0`, the routine `0x02F160` run while creature `+0x12C` is positive, and the meaning of `+0x12C`; and which creatures reach the per-creature body (Section 3 starts at the clamp).
- The world update's second argument, passed through as the behaviour's fourth argument (unused by the two classes specified).
- The decisions and draws of the other behaviour classes, and the exact element lists of ordinary records beyond the one site verified in Section 6.
- The finishing step's other kind paths, its default path and its non-zero-third-argument preparation.
- Whether any writer outside the routines examined here can set bits `0x1000`, `0x0200` or `0x0040` of the companion record's `+0x7A` before the spawn pass (Section 8 bounds only the routines examined).
- The other paths of `0x1C7730` and the later generator calls that read bit `0x0200`.
- The signedness of the creature map's key ordering (affects only the tie-break order of equally distant players with identifiers of opposite sign).
