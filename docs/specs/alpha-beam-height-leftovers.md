Status: FINAL / STABLE / HELD. Full author readback completed.

# Alpha creature drawn-height offset, beam-sibling action producers, the N = 0 case of the attack-speed query, and the beam effect particle consumer

## Scope and evidence

This contract closes, as far as the original images allow, items left open by the held contracts `alpha-combat-beam-leftovers.md` (sections 2.1, 4.2 and 10), `alpha-head-pitch-and-pick.md` (sections 2.2 and 6) and `alpha-combat-stat-queries.md`. Those three contracts were read in full and are used only to locate routines and for their terms. Every statement below was checked against the original images.

Supported images, both 32-bit with preferred base `0x00400000`. Hashes were recomputed before use:

- Client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
- Server `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`.

All addresses are RVAs. Evidence: the original instruction listings (`decompiled/client/assembly.txt`, `decompiled/server/assembly.txt`), the call lists, the decompiler's reduced pseudocode for the controller `0x088EE0` and the combat routine `0x02CB20` (used only to read switch structure; every fact taken from it was re-checked against instructions or bytes), and image bytes (constants, jump tables, and two code regions that the listing leaves undecoded, which were decoded by hand from the image: the controller's skill-release cases near `0x09BEE3` and the network receive thread near `0x06CD10`). Sites inside the world creature update `0x20C510` exist in the server at the constant offset −`0xDA240`. The isolated Infinite-Cubes reconstruction was consulted for naming leads only; such leads are marked "(reconstruction lead)".

Terms follow the held contracts: subject S, action byte `+0x68`, clock `+0x6C`, step (the world update's first argument, signed 32-bit milliseconds, stored at `0x20C54F`), s (the step converted to binary32, stored at `0x20C84A`), B, C, R0, Q (attack-speed query `0x047700`), N (query `0x044CB0`), mana `+0x170`, charge `+0x144`, stored charge `+0x13B4`, combo count `+0x70`, class pair (`+0x140`, `+0x141`), position `+0x10` (three signed 64-bit values, 65536 units per world unit; z is the third, at `+0x20`), size triplet `+0x80` (binary32; z component at `+0x88`). These are added:

- **T(x)**: binary32 x multiplied in binary32 by 65536.0, then converted to signed 64-bit by the runtime conversion `0x28D946`, which truncates toward zero.
- **F(v)**: fixed-to-float of a signed 64-bit value v: integer load to the x87 unit, one store to binary32 under the ambient x87 rounding control, then a binary32 multiply by 2⁻¹⁶ (helper `0x0120F0`).
- **Block index** of a fixed value v (helpers `0x02F100`, `0x02F040`): the signed 64-bit quotient of v by 65536 truncated toward zero (runtime division `0x28D9E0`); when v is negative, one is then subtracted. A negative exact multiple of 65536 therefore gives one less than its true floor.
- **Offset**: the binary32 drawn-height offset at creature `+0x1180`.

## 1. The drawn-height offset +0x1180

### 1.1 Where it is written (complete list)

A displacement search of both images for every access to `+0x1180`, and for wider stores that could cover it (64-bit and 128-bit moves at `+0x1170`…`+0x1180`), finds these writes and no others:

| Client | Server | Writer |
|---|---|---|
| `0x03B6FF` | `0x00646F` | Creature constructor `0x03B690` / `0x006400`: set to +0.0. |
| `0x20B9DE` | `0x13175E` | Unstuck routine `0x20AB30` / `0x1308B0`: set to +0.0 (1.4). |
| `0x21CC11` | `0x1429D1` | Step-up snap (1.2). |
| `0x21D583` | `0x143343` | Step-down snap (1.3). |
| `0x21F0C3` (helper writes through the address taken at `0x21F081`) | `0x144E41` | Decay (1.5). |
| `0x21FAFF` | `0x1458BF` | Copy from an attachment partner (1.6). |
| `0x21FD2A` | `0x145AEA` | Set to +0.0 while `+0x1398` is positive (1.6). |
| `0x221088` | `0x146E48` | Copy from an attachment partner, second pass (1.6). |

The creature reset routine `0x046330` (server `0x0110D0`) does **not** write the offset; it writes `+0x117C` and `+0x1184` only.

**Network and whole-record copies.** The offset is not part of the networked entity record. That record lies at creature `+0x10`; the record copy routines `0x04B040` and `0x066050` end at record `+0x1167` (creature `+0x1177`), and the entity field reader `0x0CCFA0` and stream receiver `0x0CD3E0` reference no record offset at or above record `+0x1170` (creature `+0x1180`). No string-move block copy with a constant count reaching `+0x1180`, and no memory-copy call with a constant length of at least `0x1184` bytes, exists in either image. The offset therefore never travels in network records; each side derives it locally from the same producers. A copy with a run-time length was not excluded exhaustively (Section 6).

### 1.2 The movement sub-step loop and the step-up snap

Every creature that reaches the movement block at `0x218F44` runs the following without any branch leaving the span `0x218F44`–`0x21F081` (both images):

1. **Step-up permission** is set at `0x218F5A`. It is cleared only at `0x21BC98` (1.2, step 5) and stays cleared for the rest of this creature's update.
2. **Displacement.** d = triplet `+0x40` × (s × 0.001) (`0x218F44`–`0x218F61`). It is set to zero when signed `+0x12C` is positive; its z is set to 0 when bit `0x0001` of word `+0x124` is clear, bit `0x10` of `+0x5C` is set and d.z > 0.0 (`0x218F69`–`0x218FC6`). Action-specific code between `0x21A1DC` and `0x21B3A6` may change d further; that code is outside this contract.
3. **Sub-steps.** n = truncation of (length of d + 1.0) (`0x21B3F5`–`0x21B426`; the length is the binary32 of the binary64 square root of the binary32 sum of squares, helper `0x023F20`). d is multiplied per component by (1.0 ÷ n). Before the loop the unstuck routine may run (1.4). If n ≤ 0 the loop is skipped.
4. **Per sub-step** (`0x21B4C0`–`0x21CCA1`): a fixed **base offset** b is set to 0 (`0x21B4C2`). Then for each axis a = x, y, z in that order (`0x21B4D5`–`0x21CC88`): position[a] += T(d[a]); the box is position ± T(0.5 × size) (each component T of the binary32 half-size), converted to an inclusive range of block indices; then the collision tests below run.
5. **Collision tests for the axis**, in this order:
   - **Terrain** (`0x21B52D`–`0x21B70E`): every block of the range, x outermost, is fetched by `0x02F7E0`. A block collides if the solidity test `0x03B480` accepts it, or, only when bit `0x0100` of word `+0x7E` is set, if its type (`0x034C10`) is not 2. On the first collision the search stops. On a horizontal axis control goes straight to the collision handling below; on the z axis it goes there only if the local "landed on an object" flag is set, and otherwise continues with the next two tests with the collision still recorded.
   - **Static chunk entities** (`0x21B72E`–`0x21BCA3`): entities held by the chunks around the subject (lists fetched by `0x02F640`; kinds 6, 7 and 9 are skipped, kinds 1, 2, 3, 5 and 8 only when their byte `+0x30` is nonzero) are tested by box overlap. A hit sets the static-hit flag (which later sets bit `0x40` of `+0x5C`, `0x21CDC3`), and on the z axis sets the landed flag when 0.0 > the vertical velocity word chosen at `0x21AB7A`–`0x21AC06` (normally `+0x3C`); in every case it **clears the step-up permission** (`0x21BC98`) and goes to the collision handling.
   - **World objects** (`0x21BCB0`–`0x21C13E`): each object in world list `+0x0C` within 4096.0 world units (binary32 at image `0x302B80`; the distance is the binary32 length of F of the 64-bit difference) is tested voxel by voxel in its own model frame (model `0xA03` of world collection `+0x1C`; object position `+0x80`, yaw `+0x98`). On a hit the collision flag is set, the subject's id is inserted into the object's set `+0x78`, and **b becomes the object's base height**: object position z − T(model height × 0.5) (`0x21C089`).
6. **Collision handling.** On the z axis (`0x21C158`–`0x21C48B`) the move is undone, the vertical velocity word and the third component of triplet `+0x4C` become 0, and `+0x13BC` receives F(position z); landing sounds and fall damage there are outside this contract. On a horizontal axis (`0x21C490`) the **step-up attempt** runs only when the step-up permission is still set and bit `0x0100` of word `+0x7E` is clear; otherwise the axis move is undone (position[a] −= T(d[a]), `0x21CC31`–`0x21CC59`).
7. **Step-up attempt** (`0x21C49E`–`0x21CAFD`). The box is raised by T(1.01) on z (1.01 is binary32 `0x3F8147AE`): its corners are (position + (0, 0, T(1.01))) ∓ T(0.5 × size). The attempt fails if any block of that range is accepted by `0x03B480` (`0x21C680`–`0x21C69E`), or if any world object of list `+0x0C` within 4096 units has a voxel in the raised box (`0x21C720`–`0x21CAB8`; a hit there also inserts the subject's id into the object's set `+0x78`). A failed attempt undoes the axis move.
8. **Step-up snap** (`0x21CB09`–`0x21CC23`), when the attempt succeeds. With z the current position z (after the axis move), h = binary32 (size z × 0.5), and r = z − b:
   - the new relative height is 65536 × (block index of (r − T(h))) + T(h) + 66191. The last term is the negation of the truncated binary64 product 1.01 × (−65536.0) (constants at image `0x31E1F8` and `0x302AE0`);
   - the offset becomes F((new relative height) − (r − T(offset))), that is, the snap distance plus the previous offset, so z − offset is continuous across the snap;
   - position z = new relative height + b.

   The axis move is kept and velocity is not changed. The loop continues with the next axis.

The step-up snap can therefore happen once per horizontal axis per sub-step. Its target is the top of the block (of the terrain grid, or of the touched object's grid when b is nonzero) that contains the subject's lowest point, plus 66191/65536 world units plus half the subject's height.

### 1.3 The step-down snap

After the sub-step loop (`0x21CCA7`), the subject's previous ground bit (bit `0x01` of `+0x5C` as left by the previous update) is captured, and `+0x5C` is cleared (`0x21CCEB`). A block-type probe over the post-move box (`0x21CDC7`–`0x21D0C6`; it sets bits `0x10` and `0x02` of `+0x5C` from the block types of `0x034C10`, bit `0x02` when a type-2 block is found) and, only when bit `0x02` is clear, a ground probe of the single block layer at block index (z − T(h) − T(0.2)) (`0x21D1DF`–`0x21D31C`; a block accepted by `0x03B480` sets bit `0x01`) run first. Then the step-down snap (`0x21D38F`–`0x21D5AA`) runs when all of these hold:

1. the captured previous ground bit is set;
2. the step-up permission is still set;
3. the per-sub-step displacement's horizontal squared length (d.x² + d.y², helper `0x024830`) is greater than 0.0;
4. bits `0x01` and `0x02` of `+0x5C` are both clear (neither probe of this update set them);
5. 0.0 > d.z is false (d.z ≤ 0.0; a NaN fails).

Then one block layer, at block index L = block index of (z − T(h) − T(1.2)) (1.2 is binary32 `0x3F99999A`), is searched over the post-move box's x and y block range, x outermost. At the first block accepted by `0x03B480`:

- bit `0x01` of `+0x5C` is set;
- the new z is 65536 × L + T(1.01) + T(h), where T(1.01) = 66191 (the same value as in 1.2);
- the offset becomes F((new z) − (old z − T(offset)));
- the search ends.

There is no base-offset term here; the snap is on the terrain grid only. It happens at most once per update. This and 1.2 are the exact snap targets and conditions left open by the head-pitch contract (section 2.2).

### 1.4 The unstuck routine 0x20AB30

Called once per update, before the sub-steps, at `0x21B47E` when (world byte `+0xB4` is 0, or S is the world's local creature `+0xB8`) and byte `+0x60` is not 6. It returns at once when 0.0 > `+0x16C`. Otherwise it scans every block of the box position ± T(0.5 × size). If any block's type (low five bits of block byte 3, read inline; missing columns use the image's static blocks) is neither 0 nor 2, position z becomes 65536 × (block index of (z − T(h))) + T(h) + 72089 (72089 is the truncation of binary32 72089.6, image `0x31E20C`), and the offset is set to +0.0 (`0x20B9DE`): a hard lift with no smoothing. The scan runs at most once per call. The rest of the routine (`0x20B03F` onward, world-object handling) does not touch the offset.

On the client (world `+0xB4` nonzero; reconstruction lead "client world") this runs only for the local player. On the server it runs for every creature whose byte `+0x60` is not 6.

### 1.5 Decay and the repeat count

At `0x21F081`–`0x21F0C3` the offset approaches 0.0 through the repeated-approach helper `0x011D40` with target 0.0 and **repeat count = step** (the world update's first argument, unchanged since `0x20C54F`). The rate is 0.01 (`0x3C23D70A`) when 0.0 > offset (ordered), and 0.02 (`0x3CA3D70A`) otherwise, including NaN. Each repetition computes offset = ((0.0 − offset) × rate) + offset, each operation rounded to binary32. A step of 0 or less makes no repetition. Immediately before, at `0x21F058`–`0x21F07E`, `+0x1188` decays the same way with the same repeat count and rate 0.005; immediately after, `+0x1184` with rate 0.0075 (both outside this contract).

### 1.6 Copies and the remaining zeroing

- **Attachment copy** (`0x21F968`–`0x21FB75`). When byte `+0x60` is 5 and the creature whose id is at `+0x198` exists and has action `0x6A`, S is attached to it: among other fields, `+0x1180`, `+0x1188` and `+0x118C` are copied from it (`0x21FAF3`–`0x21FB32`). The same copy happens in a later pass over the creature map, after the creature loop and the projectile update (`0x220E90`–`0x221136`; copy at `0x221082`–`0x2210A0`).
- **Interpolation reset** (`0x21FB80`–`0x21FD34`). Otherwise, while signed `+0x1398` is positive, the drawn position and rotation approach their targets and the offset is set to +0.0 (`0x21FD2A`).

**Random draws:** none in Section 1. The fall-damage and sound code in 1.2 step 6 is outside this contract.

## 2. Actions 0x5E and 0x68

### 2.1 Every store to the action byte

A search of both images for byte, word and dword stores to `+0x68` (immediate, register, read-modify-write and string stores) gives these sites besides network and copy paths:

- Immediate stores of `0x68`: **one** per image, `0x02E66F` (server `0x004A8F`), in the combat routine (2.3). The controller store at `0x09BEE9` lies in a region the listing does not decode; it was decoded from the image bytes (2.2).
- Immediate stores of `0x5E`: **none**, in either image, of any width.
- Register stores: `0x0455D7` (value from `0x045020`), `0x0458A0` (value from `0x0452A0`), `0x09BD95` (a skill-bar entry, 2.2), `0x02D870`, `0x02D944`, `0x02DA5F`, `0x02DD59`, `0x02EA92` (combat routine, 2.3), `0x0DB1A3` and `0x2070BD` (both store 0), `0x21A137` and `0x21A14C` (values 5, `0x11` or `0x14`). The server's search gives the counterparts of these sites (for example `0x0105C7`, `0x010890`, `0x01BC93`, `0x13FEF7`, `0x13FF0C`, and the combat routine's stores) and no other register store of a nonzero value.

`0x045020` (server `0x010010`) returns only `0x01`/`0x02`, `0x06`/`0x07`, `0x09`/`0x0A`, `0x0D`/`0x0E`, `0x12`/`0x13`, `0x16`, `0x1A`, `0x1E`/`0x20`, `0x26`/`0x2C`, `0x27`/`0x28`, `0x29`/`0x2A`, `0x39`/`0x3A`/`0x43` and `0x4B`. `0x0452A0` (server `0x010290`) returns only `0x03`/`0x04`, `0x05`, `0x17`, `0x1F`/`0x21`, `0x25`/`0x2B`, `0x2D`/`0x2E` and `0x41`/`0x42`. Neither returns `0x5E` or `0x68`.

### 2.2 Local player (client controller 0x088EE0)

**Skill bar** (`0x08B8F0`–`0x08BD40`). The controller rebuilds an integer list at `+0x800814`. Entry 0 and entry 1 depend on the subtype byte `+0xAA9` of slot `+0xAA8` (its kind is not tested): 10 gives (`0x1E`, `0x1F`), or (`0x20`, `0x21`) when `+0x141` is 1; 12 gives (`0x28`, `0x25`) or (`0x2A`, `0x2B`); 11 gives (`0x26`, `0x2E`) or (`0x2C`, `0x2D`). Otherwise entry 0 is `0x045020`'s choice and entry 1 is, in order of precedence: 5 if the subtype is 5; `0x11` if 3; `0x14` if 4; `0x19` if it is 6 or byte `+0x991` is 6; `0x18` if it is 7 or `+0x991` is 7; `0x1B` if 8; `0x3B` if `0x044230` holds (slot `+0xAA8` kind 3 with subtype 5, 6, 7, 8, 10, 11, 15, 16, 17 or 18); **8 if byte `+0x991` (subtype of slot `+0x990`) is `0x0D`** (`0x08BB14`); `0x3F` otherwise. Entries 2–4 depend on the class byte `+0x140` and `+0x141`: 3 gives `0x22` or `0x58`, `0x67`, `0x31`; 4 gives `0x30`, `0x4F`, `0x61` or `0x60`; 2 gives `0x15`, `0x32`, `0x64` or `0x63`; 1 gives `0x36`, `0x56`, `0x66` or `0x65`. Entries from index 2 onward (up to index 6) are set to 0 when the matching signed counter at `+0x1150` + 4 × (index − 2) is below 1. **No path puts `0x5E` into the bar.** The release form `0x68` arises only from entry 1 = 8.

**Input processing** (`0x09B5B1`–`0x09C02F`; skipped while controller byte `+0x800704` is set). For each bar index i from 1 to the last, with input byte at controller `+0x04` + i:

- **Pressed.** If the entry is nonzero, and (`0x043E350` holds or i ≥ 2), and `0x043E5A0`(entry) holds, the entry is dispatched by the table at `0x09D57C`/`0x09D52C` (`0x09B6C1`–`0x09B6E2`). Entries `0x03`, `0x04`, `0x17`, `0x1F`, `0x21`, `0x25`, `0x2D`, `0x2E`, `0x41`, `0x42`, `0x5E` and `0x5F` call `0x0456A0`, which sets the action to `0x0452A0`'s choice, never to the entry itself. Entries 8, `0x18`, `0x19`, `0x1B`, `0x3B`, `0x3F` and `0x40` start that action with clock 0 unless it is already the current action. The default case sets the action to the entry when mana ≥ the cost query (`0x09BD43`–`0x09BD9E`).
- **Released.** If the input byte is 0 and the entry equals the current action, the entry minus 8 selects a case through `0x09D610`/`0x09D5E4` (`0x09BDBA`–`0x09BDF9`). **Entry 8 sets the action to `0x68` and the clock to 0** (`0x09BEE3`–`0x09BEF5`, store at `0x09BEE9`). This path tests no mana, no charge, no cooldown and no equipment beyond the bar entry.

### 2.3 Non-player creatures (combat routine 0x02CB20, server 0x002F40)

`0x02CB20` is the first virtual function of a 20-byte behaviour object (vtable at `0x2FD310`, constructor `0x02C550`). The image's run-time type descriptor at `0x3658D0` names its class `CombatBehavior`. Its server counterpart matches it after masking addresses, except for a call-cleanup difference near `0x02D957` that does not affect action values.

**Action 0x68** (`0x02E25E`–`0x02E68E`). Reached when signed `+0x64` is not `0x68`; subtype byte `+0xAA9` is not 6, 7 or 8; bits `0x10` and `0x08` of word `+0x7E` are clear; and the squared separation to the current target, formed earlier in the routine, is less than ((S `+0x80` + target `+0x80`) × 3.0)². Then:

1. If the action is not `0x3B`, `0x3F` or 8 and `+0x128` is 0, one `rand` draw is made; when its remainder by 10 is 0 and further conditions hold, a sidestep is chosen instead (outside this contract).
2. If mana widened to binary64 is less than 0.8 (binary64, image `0x2FD320`) and the action is not `0x3B`, `0x3F` or 8, another branch is taken (`0x02E2D3`).
3. Otherwise, only if `+0x128` is 0 and the clock is greater than R0: if `+0xAA9` is 5, the action becomes 5 (clock 0). Else, if slot `+0xAA8`'s kind byte is 0 or slot `+0x990`'s kind byte is nonzero, and `+0x991` is `0x0D`:
   - if the action is 8 and charge ≥ mana (ordered; a NaN fails), the action becomes **`0x68`** and the clock 0 (`0x02E66F`);
   - if the action is 8 and charge < mana, the action stays 8 and the clock is kept;
   - if the action is not 8, the clock becomes 0 and the action 8.

The reconstruction calls action 8 a shield block and `0x68` its release strike (reconstruction lead).

**Action 0x5E.** The routine's random choices come from candidate sets built in place: {`0x45`, `0x4A`, `0x49`}; {7, 6, `0x14`, and `0x5F` when 16-bit `+0x8C` is `0x86A`, otherwise `0x56`}; {`0x6C`, `0x36`, `0x4C`}; {`0x44`, `0x45`, `0x46`, `0x4A`, `0x4B`, `0x4D`, `0x4E`, `0x4C`, `0x47`, `0x48`, `0x49`} (subsets at `0x02DB19`–`0x02DD5C`); and the creature's special-skill list. That list (`+0x13E8`/`+0x13EC`, picked at `0x02D494`–`0x02D4C0`, applied at `0x02D870`) is copied at spawn (`0x21037D`) from the spawn record's list `+0x10AC`. That list is filled only by `0x1C7730`, with values `0x45`, `0x56`, `0x57`, `0x5B`, `0x5C`, `0x5D`, `0x5F` and a random one of {`0x5C`, `0x5D`, `0x56`, `0x59`, `0x57`}. **None of these is `0x5E`.** The immediates `0x5E` at `0x1CC856` and `0x1DCBCF` are race codes: they enter lists from which the spawn record's `+0x2C` is drawn (`0x1D3BEC`), which the world update copies to creature `+0x64` (`0x210031`). The reconstruction names race 94 "Werewolf" (reconstruction lead). The spawn record's byte `+0x58` goes to creature `+0x1A8` (`0x21004C`), not to the action. Because 2.1 enumerates every store to the action byte, the remaining `0x5E` immediates (exception-state bytes, table data, comparisons and call arguments) cannot set it.

### 2.4 Network and copies

The action byte is entity-record `+0x58`. It is read by the field reader (`0x0CD085`) and the stream receiver (`0x0CD54E`) and copied by `0x04B040` and `0x066050`. These carry whatever value a peer sends; they choose no action.

**Result.** In neither image does any code choose action `0x5E` for any creature. It can reach a creature only through received or copied entity data. Its handlers (world-update ray case, cost and timing queries, the controller's dispatch group, cast-bar label, renderer selectors at `0x0AFFBD` and `0x0B4697`) are therefore unreachable from original-image choices alone. Action `0x68` has exactly two producers: the local player's release of bar entry 8 (client only), and the combat routine for non-player creatures (both images; where the routine is invoked, and on which side, was not traced).

### 2.5 Costs and durations (from the held stat-queries contract, re-checked)

- **Cost** (`0x044AE0`): 0.0 for `0x5E` (default outcome), and 0.0 for `0x68` (index `0x65` is above `0x5C`).
- **0x5E**: B = scaled 500, C = scaled 500, D = scaled 100. So R0 = s500 (B term) + s500 (C term) + s100 (D term), where sK is the truncation of K ÷ (Q × `+0x17C`), each term evaluated separately and the sum wrapping.
- **0x68**: B = fixed 0, C = scaled 300, D = scaled 100. So R0 = 0 + s300 + s100. Because B is 0, the first crossing is the update in which the clock is 0 and the step is positive.
- **Action 8** (for 2.3): B = fixed 0, C = scaled 300, D = scaled 500.

### 2.6 The charge transferred at clock 0 of action 0x68

The beam contract's section 1.1 moves the charge (binary32 `+0x144`, which is entity-record `+0x134`) into mana and `+0x13B4`. Writers of `+0x144`:

- **Hold accumulation** (world update `0x21A78C`–`0x21A8F5`). While the action is `0x18`, `0x19`, `0x1B`, 8, `0x3B`, `0x3F` or `0x40` and the clock is at least B of that action, rate = Q × (s × 0.00075). For class pair (2, 0) the rate is multiplied by (min(combo count ÷ N, 1.0) + 1.0), with the same division form as Q (Section 3). When the effect list holds a kind-2 element, the rate is multiplied by ((`0x03ED60`(`0x66`, −1) × 9.0) + 1.0). The charge becomes rate + charge. If the effect list holds a kind-10 element, the charge is then set to mana.
- **Action 0x24 accumulation** (`0x21A74D`–`0x21A784`): charge = (Q × (s × 0.0005)) + charge.
- **Clamp** (`0x21A8FB`–`0x21A910`, every creature that reaches it): if charge > mana (ordered), charge = mana. With this clamp, the release condition of 2.3 (charge ≥ mana) means a full charge.
- **Set to 0**: `0x215CD1` (the `0x68` transfer itself); `0x213608`, `0x21714C` (after copying the charge to `+0x13B4`), `0x2176A9` and `0x218191` in other action cases; `0x21A25C` (`+0x128` nonzero, with the exceptions tested at `0x21A211`–`0x21A256`); `0x21AE66` (signed `+0x12C` positive).
- **Gain from received hits.** The local player on the client (`0x08FF3D`–`0x090035`): for a hit record addressed to the local player whose byte `+0x45` is 1, mana and charge each gain `+0x174`, mana is clamped to 1.0 and then charge is clamped to mana. Hit routine `0x196D30` (`0x197034`–`0x1970B0`; server `0x0CFD50`): when `+0x174` > 0.0, the same pair of gains and clamps. Its reach conditions were not traced.
- **Record initialisation and copies**: set to 0 by `0x03C100` (`0x03C1E8`, from the constructor) and `0x0465C0` (`0x0466AD`, from the reset routine); copied by `0x04B040` (`0x04B169`) and `0x066050` (`0x066178`); networked (field reader `0x0CD1A5`, receiver `0x0CD729`).

World-update sites have server counterparts at −`0xDA240`. **Random draws:** only the combat routine's rand of 2.3 step 1; none in 2.2 or 2.6.

## 3. Query 0x047700 when N = 0

N (`0x044CB0`) is the truncating conversion of binary32 `+0x17C` × k. It is 0 exactly when −1 < `+0x17C` × k < 1, including ±0 (k from the beam contract, 2.1). N is evaluated only for class pairs (1, 0) (`0x047740`–`0x0477A5`) and (3, 1) (`0x0477AA`–`0x047804`). With c the combo count converted to binary32 and the SSE division under the default masked exception state:

- **c > 0**: c ÷ 0.0 = +∞; the test +∞ > 1.0 holds, so the ratio is 1.0. Q is then the same as with a saturated ratio: a = (1.0 × 0.5) + a for (1, 0), or 1.0 + a for (3, 1).
- **c = 0**: 0.0 ÷ 0.0 is the processor's default quiet NaN (bit pattern `0xFFC00000`). The comparison with 1.0 is unordered, so the unclamped branch recomputes N (still 0) and divides again, giving NaN. a becomes NaN, t = E + a is NaN, and the kind-`0x0C` factor, when present, multiplies NaN. **Q is NaN.**
- **c < 0** (no known writer produces this): −∞, not clamped, so a and Q become −∞.

**Propagation into the held contracts' consumers:**

- **B, C, R0.** Each scaled term is the truncation of K ÷ (Q × `+0x17C`). A NaN Q gives NaN and the conversion's integer indefinite `0x80000000`. R0 adds up to three such terms with wrap-around (two give 0, three give `0x80000000`). For c > 0 a finite Q gives an ordinary value, or `0x80000000` when the quotient exceeds the 32-bit range.
- **Clock rescale** (beam contract 2.2). The second query is evaluated with the combo count already 0, so it is NaN for these class pairs whenever N = 0. The quotient and product are NaN and the **clock becomes `0x80000000`**, whatever the first query was.
- **Charge accumulation (2.6).** It multiplies by Q, so a NaN Q makes the charge NaN. The clamp (charge > mana) is unordered and does not repair it. At the next `0x68` transfer, mana − NaN is NaN and the "less than 0.0" clamp does not repair mana. The class-pair (2, 0) factor in 2.6 divides combo count by N in the same way.

**Equipment-slot order** (`0x0478B0`, `0x047955`–`0x047AF3`) is confirmed: `+0x990` kind 3, `+0xAA8` kind 3, `+0x530` kind 4, `+0x648` kind 6, `+0x760` kind 5, `+0x878` kind 7, `+0x418` kind 8, `+0xBC0` kind 9, `+0xCD8` kind 9. Each matching slot's contribution `0x0C7C00` is added as (contribution + running value). This agrees with the held stat-queries contract (sections 2 and 3.2) and the beam contract (2.1).

## 4. Client presentation of the 72-byte effect record

### 4.1 Per-particle construction (controller `0x0952A5`–`0x0955AB`)

For each record of the controller's output list `+0x10` (the output object passed to the world update at `0x08CF2E`/`0x08CF49`), in list order, and for each of the record's `+0x38` repetitions (none when it is 0 or less):

1. A particle is constructed by `0x065DE0`. Its defaults: `+0x00` and `+0x04` = 0; `+0x38`, `+0x3C`, `+0x40` = 0; `+0x54`…`+0x68` = 0; `+0x6C` = 4×4 identity; `+0xAC` = 1.0; 16-bit `+0xB0` = 0; `+0xB4` = 4000.
2. `+0x00` (32-bit) = record `+0x3C` (kind); `+0x04` = 0; `+0x08` (3 × signed 64-bit) = record `+0x00`.
3. **Draws 1, 2, 3**: each rᵢ = (rand as binary32 ÷ 32767.0) − 0.5. V = (r₃, r₂, r₁), so draw 1 feeds z, draw 2 y and draw 3 x. f = record `+0x40` × 2.0. **Velocity** `+0x54` = per component (Vᵢ × f) + record `+0x18`ᵢ, each operation rounded to binary32.
4. `+0x38` triplet = (0, 0, 0).
5. **Draws 4, 5, 6**, same form: W = (r₆, r₅, r₄). **Third triplet** `+0x60` = W × 100.0 per component.
6. **Colour** `+0x44`…`+0x53` = the four binary32 words at record `+0x24`, copied raw.
7. If the kind (32-bit compare) is 1: **draw 7**, u = rand as binary32 ÷ 32767.0 (no subtraction), and the colour is replaced with (1.0, u, 0.0, 1.0). Draw 7 therefore feeds the green component.
8. **Size** `+0xAC` = record `+0x34`. `+0x6C` is set to identity again.
9. The particle is appended to the controller's list `+0x800740` (`0x0869D0`).

The beam record has kind 1, a count of 5, f = 3.0 × 2.0 = 6.0 and size 0.1. Each such record costs 5 × 7 = 35 draws in the order (r₁…r₆, u) per particle, with particles in repetition order and records in list order.

### 4.2 Received records and the fourth colour word

**Receive path.** The client's network thread `0x06B740` reads an update into a local object laid out like the world-update output. The 72-byte reader `0x058860` is called at `0x06CD10` and fills its list `+0x10`. Between calls to `0x201CB0` and `0x201E90` on controller `+0x2E4`, each list's elements are copied onto the end of the controller inbox list at `+0x800630` + list offset; list `+0x10` goes to `+0x800640` (`0x06CEDD`–`0x06D088`, range insert `0x057B30` and siblings). In the controller update, `0x0904CE`–`0x09050B` copies the whole inbox list `+0x800640` onto the end of the local output object's list `+0x10` (range insert `0x057D10` at the list end), after the world update has filled it; the inbox lists are then cleared (`0x06F870`, `0x0905C4`–`0x0905E9`). So **received records are presented by the same consumer**, after the locally produced ones and in received order, with the same 35 draws per beam record.

**Fourth colour word.** In the world update the record's colour is copied as four words (`0x216AA5`, helper `0x058CF0`) from a temporary built with only three components (`0x216A5E`, constructor `0x00EA90` with 1.0, 0.2, 0.5). The fourth word is the update frame's word at frame offset −`0x1BE0`, which no instruction of the world update writes by displacement. Its value is whatever earlier stack use left there and is not statically determinable. It is copied into the particle and then **always overwritten**, because the beam record's kind is 1. It has no effect on presentation. It does travel in the network bytes (beam contract 4.2), on both sides, since the record builder is identical in the server.

The server has no particle consumer.

## 5. Client/server agreement

- These world-update ranges match the server at −`0xDA240` instruction for instruction after masking absolute addresses: `0x218F30`–`0x21F100`, `0x21F100`–`0x222D63`, `0x21F960`–`0x21FB80`, `0x21FD20`–`0x21FD50` and `0x220EE0`–`0x221110`. They cover every site of Sections 1 and 2.6.
- The unstuck routine `0x20AB30` matches `0x1308B0`; only the addresses of the static blocks differ.
- `0x045020`–`0x045D70` match `0x010010` onward, differing only in jump-table addresses. The combat routine's `0x68` path `0x02E25E`–`0x02E6B0` matches `0x00467E` onward in the same way.
- The query `0x047700` and its callees match their server counterparts (held contracts).
- Client only: the controller (skill bar, input release, hit-record charge gain), the network receive thread and inbox, and the particle consumer.

## 6. Unresolved

- Whether any copy with a run-time length (rather than a constant one) can carry `+0x1180`. None was found by the searches of 1.1.
- The action-specific changes to the displacement (`0x21A1DC`–`0x21B3A6`); the static-entity kinds and their box test; the world-object voxel test `0x0A2870`/`0x028940`; the block helpers `0x02F7E0`, `0x03B480` and `0x034C10`; and the rest of `0x20AB30`.
- The meaning of `+0x5C` bits beyond their producers here, of word `+0x7E` bit `0x0100`, of `+0x1398`, `+0x198`, action `0x6A`, the world `+0x0C` objects and model `0xA03`.
- Where and on which side the combat routine is invoked; the earlier part of `0x02CB20` (target choice and the separation it compares); the sidestep branch; and the meaning of `+0x64` value `0x68` and the `+0x8C` value `0x86A`.
- The skill-usable tests `0x043E350` and `0x043E5A0` in the controller's press path; the gameplay meaning of actions 8, `0x5E` and `0x68`; and why the image handles `0x5E` with no producer.
- The reach conditions of the charge gain in hit routine `0x196D30`, the meaning of `+0x174`, and the kind-2 and kind-10 effect elements of 2.6.
- The value of the fourth colour word, which depends on prior stack contents.
- The particle fields `+0x38`, `+0x60`, `+0xB0` and `+0xB4` beyond their initial values. How the particle list is advanced and drawn is outside this contract.
- Behaviour of Q when N is negative or when `+0x17C` × k is NaN or out of range (N = `0x80000000`). The exception-mask and rounding state is assumed to be the default; a program change of MXCSR was not searched.
