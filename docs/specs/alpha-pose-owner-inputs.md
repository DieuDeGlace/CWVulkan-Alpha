Status: FINAL / STABLE / HELD

# Alpha pose owner inputs: writers of the creature fields read by the pose refresh

This source-side contract describes the creature ("owner") fields that the client pose refresh RVA `0x0128F0` reads and that the held pose contracts leave open. For each field it gives the storage, the writers found in the client image, the values written, the initial value, and whether the field travels in the network creature record. Field names are storage descriptions only. Reconstruction names are quoted as leads and are not asserted. Items that could not be proved are kept in the Unresolved section.

## 1. Evidence and method

- Supported image: client `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` (verified for this contract), preferred base `0x00400000`. All addresses are RVAs (image virtual address minus `0x400000`). "Image `X`" names data at RVA `X`, and "import slot `X`" names an import address table entry; its name was read through the slot's hint/name entry in the image.
- Listed code was read from `decompiled/client/assembly.txt`. Call relations came from `decompiled/client/calls.csv`. Constants were read from the image bytes through the section table. No decompiler output was used.
- The listing does not cover all code. A byte comparison of the listing against `.text` found 923 unlisted runs of at least 16 bytes, each holding at least 8 bytes that are not padding. Every such run was disassembled directly from the image bytes with the Microsoft `dumpbin /disasm` tool (Visual Studio 2022 Build Tools, MSVC 14.44.35207). The resulting text was kept only in scratch storage. Selected listed ranges were also re-read with the same tool. Some unlisted runs are data, and their decodings are meaningless byte-pair "additions". Those were recognised as such and ignored.
- Writers were found by scanning every listed and newly decoded instruction whose destination is a memory operand based on a general register (not the stack or frame pointer) and that carries the field's displacement. Each field was scanned both as a creature offset and as an offset from creature `+0x10` (Section 2). The surroundings of each candidate were then read to identify the base object. A candidate is attributed to a creature only when the base is shown to be one: a creature constructed by RVA `0x03B690`, the creature pointer held at client `+0x8006D0`, the creature in the world-update frame slot `−0x2B14`, a creature block reached through `+0x10`, or a creature pointer passed to a routine shown here to act on one. All other candidates are unattributed.
- The Infinite-Cubes reconstruction (`alpha/include/cube/Creature.h`, `alpha/src/cube/Network.cpp`, and a search of `alpha/include/cube/SpriteIds.h` for two numbers) was consulted for layout and naming leads only. Every offset, size, order and value stated below was read from the original.

## 2. Storage frame and the network creature record

**Entity block.** The creature constructor RVA `0x03B690` calls RVA `0x03C100` with the address creature `+0x10` (RVAs `0x03B6CB`–`0x03B6D1`). This contract calls the block initialised there the *entity block* E (E offset = creature offset − `0x10`).

**Record readers.** RVA `0x0CCFA0` reads an entity block from a byte buffer, and RVA `0x0CD3E0` reads one from a socket through equivalent per-size helpers. RVA `0x0CDAA0` compares two entity blocks field by field and emits the changed fields (called at RVA `0x069F81` inside RVA `0x069C10`). In RVA `0x0CCFA0`:

- The first 8 bytes of a record are a 64-bit field mask.
- The fields are then visited in a fixed order. A per-size helper tests mask bit k, where k is the visit index counted from 0. When the bit is set, the helper copies the field's bytes from the current payload position and advances the position by the field size. The bit index advances whether or not the bit is set. The 4-byte helper RVA `0x0CBFC0` was read in full. When the payload is short, it sets the read position to the end and still reports the field size.
- A field's offset inside a record is therefore 8 plus the sizes of all earlier fields whose bits are set.

Visit order, written as bit: E offset (creature offset), read from RVAs `0x0CCFD6`–`0x0CD3BF`:

| Bits | Fields |
|---|---|
| 0–4 | 0: `+0x0`; 1: `+0x18`; 2: `+0x24` (creature `+0x34`); 3: `+0x30` (creature `+0x40`); 4: `+0x3C` |
| 5–9 | 5: `+0x48` (creature `+0x58`, 4 bytes); 6: `+0x4C` (creature `+0x5C`); 7: `+0x50` (1 byte); 8: `+0x54` (creature `+0x64`, 4 bytes); 9: `+0x58` (creature `+0x68`, 1 byte) |
| 10–13 | 10: `+0x5C` (creature `+0x6C`); 11: `+0x60`; 12: `+0x64`; 13: `+0x68` (creature `+0x78`; contains creature `+0x7E`) |
| 14–20 | 14: `+0x114` (creature `+0x124`); 15 to 20: `+0x118`, `+0x11C`, `+0x120`, `+0x124`, `+0x128`, `+0x12C` |
| 21–26 | 21: `+0x130` (creature `+0x140`, 1 byte); 22: `+0x131` (creature `+0x141`, 1 byte); 23: `+0x134` (creature `+0x144`, 4 bytes); 24: `+0x138` (creature `+0x148`, 12 bytes); 25: `+0x144` (creature `+0x154`, 12 bytes); 26: `+0x150` (12 bytes) |
| 27–42 | `+0x15C`, `+0x160`, `+0x164`, `+0x168`, `+0x17C`, `+0x17D`, `+0x180`, `+0x184`, `+0x188`, `+0x190`, `+0x198`, `+0x19C`, `+0x1A0`, `+0x1B0`, `+0x1CC`, `+0x1C8` |
| 43 | `+0x1D8` (creature `+0x1E8`, `0x118` bytes, copied by the runtime memory copy in helper RVA `0x0CC3C0`) |
| 44–47 | `+0x2F0`, `+0x1158`, `+0x1128`, `+0x1154` |

Sizes not stated were not needed here.

**Where decoded records land.** Both readers are called only from unlisted code inside one routine of the network receive path. Their call sites are RVA `0x06BC82` (buffer form) and RVA `0x06C41C` (socket form).

- Buffer form (RVAs `0x06BC26`–`0x06BDFF`).
  - A creature is looked up or newly constructed by RVA `0x03B690` (call at `0x06BC26`) and stored through RVA `0x068AD0`.
  - A temporary entity block is built as a copy of that creature's block: RVA `0x066050` is called with the temporary as receiver and creature `+0x10` as argument (`0x06BC62`).
  - RVA `0x0CCFA0` then decodes the record into the temporary. Fields whose mask bits are clear therefore keep the creature's current values.
  - The routine compares the creature with the creature pointer at client `+0x8006D0`. If they are the same, it skips the copy-back (jump at `0x06BCCB` to `0x06BE04`). Otherwise it adjusts the creature's drawn-position fields (`+0x1338`–`+0x1398`, outside this contract) and copies the whole temporary into creature `+0x10` with RVA `0x04B040` (call at `0x06BDFF`).
- Socket form. RVA `0x0CD3E0` decodes into an entity block obtained from a container through RVA `0x068B70` (receiver frame `−0x1204`, call at `0x06C40E`). The socket is the word at client `+0x8006CC`. Later in the same routine, creatures are constructed (`0x06C674`) and compared with client `+0x8006D0` (`0x06C69A`). When the creature is not that one, its block is filled from the container's block by RVA `0x04B040` (call at `0x06C96C`, receiver creature `+0x10`). When it is that creature, the copy is skipped (jump at `0x06C6A0`).
- Therefore every entity-block field of this contract, on every creature except the one at client `+0x8006D0`, is overwritten whenever such a record is applied. Each field then takes either the record value or its own previous value, the latter when the field's bit is clear.
- Calls to RVAs `0x201E90` and `0x201CB0` on client `+0x2E4` bracket these steps (for example `0x06BC6F`, `0x06BCBE`, `0x06BE0C`). Whether these are lock operations was not established.

**Copy routines.** Both routines copy an entity block field by field from the argument into the receiver.

- RVA `0x04B040` stores its receiver's fields from its stack argument (from `0x04B040`; bytes `+0x130` and `+0x131` and the words from `+0x134` at RVAs `0x04B149`–`0x04B1E1`). Direct callers: RVAs `0x04BF93`, `0x066E2D`, `0x06A00A`, `0x070343`, `0x0823B1`, `0x08762B`, `0x06BDFF`, `0x06C331`, `0x06C96C`.
- RVA `0x066050` behaves the same way (from `0x066050`; bytes `+0x130` and `+0x131` at `0x06615F` and `0x06616C`). Direct callers: RVAs `0x05230D`, `0x068C10`, `0x069F19`, `0x070221`, `0x06BC62`.

**Fields outside the record.** `+0x1314` and `+0x13B4` lie beyond the entity block, and no visit in RVA `0x0CCFA0` addresses them. They are client-local.

## 3. Construction and resets

- **Entity-block constructor** RVA `0x03C100`, with receiver creature `+0x10`:
  - E`+0x48` = 0 (creature `+0x58`, RVA `0x03C183`) and E`+0x54` = 0 (creature `+0x64`, `0x03C195`).
  - E`+0x68` is constructed by RVA `0x028750`, which zeroes a register at `0x02875A` and stores its 16-bit value at offset 6 (`0x02875C`). Creature `+0x7E` is therefore 0.
  - After the register is zeroed at `0x03C1BA`, the routine stores 16-bit 0 at E`+0x130` (creature bytes `+0x140` and `+0x141`, `0x03C1E1`), 32-bit 0 at E`+0x134` (creature `+0x144`, `0x03C1E8`), and 0 in each word E`+0x138`–`+0x158` (creature `+0x148`–`+0x168`, `0x03C1F8`–`0x03C228`).
  - It then stores 16-bit 0 at E`+0x1D8` (creature bytes `+0x1E8` and `+0x1E9`, `0x03C2EF`). The register is unchanged between `0x03C1BA` and `0x03C30F`.
- **Creature constructor** RVA `0x03B690`: `+0x1314` = 0 (`0x03B822`) and `+0x13B4` = 0 (`0x03B915`).
- **Reset A** RVA `0x046330`. It calls RVA `0x0465C0` on creature `+0x10`. That routine zeroes a register at `0x046697` and stores it, unchanged, in E`+0x60`, `+0x64`, `+0x134` and other words, and in E`+0x138`–`+0x14C` (creature `+0x148`–`+0x15C`, `0x0466DD`–`0x0466FB`). Reset A then sets `+0x1314` = 0 (`0x0463AF`) and `+0x13B4` = 0 (`0x04649D`). Its callers are `0x04BE7F` (in `0x04BE40`), `0x083F73` (in `0x083E70`), and the world update at `0x20E5EF`, `0x20E839` and `0x20EADF`.
- **Reset B** RVA `0x047110`. It sets `+0x1314` = 0 (`0x047120`), then calls RVA `0x047270` on creature `+0x10`. That routine zeroes a register at `0x0472AF` and stores it in E`+0x60`, `+0x64`, `+0x12C`, `+0x160` and in E`+0x138`–`+0x14C` (creature `+0x148`–`+0x15C`, `0x0472E9`–`0x047307`). Reset B then sets `+0x13B4` = 0 (`0x0471C0`). Its caller is `0x097395` in the client update RVA `0x088EE0`.
- Creature memory comes from an allocator that does not clear it (held weapon contract, Section 11). Fields that none of these routines store start with allocator bytes.

## 4. Hand-turn sources: owner `+0x148` and `+0x154`

- **Storage.** Two binary32 triplets: `+0x148`/`+0x14C`/`+0x150` (E`+0x138`) and `+0x154`/`+0x158`/`+0x15C` (E`+0x144`).
- **Record.** Bits 24 and 25, 12 bytes each.
- **Initial value.** All six words are zero (entity-block constructor).
- **Writers.** Across listed and decoded code, the only writers attributable to a creature are:
  - the constructor and Resets A and B, all writing zero;
  - the entity-block copies of Section 2;
  - application of a record (Section 2): the payload bytes for bits 24 and 25 are taken unchanged.
- **Excluded or unattributed candidates.** Other instructions store at these displacements in RVAs `0x0015C0`, `0x001930`, `0x003560`, `0x059C40`, `0x227260`, `0x227450`, `0x24D0F0`-range code (decoded stores at `0x24D936` and `0x24D97E`), `0x26362D`/`0x263637`, `0x278422`–`0x2793A6`, and elsewhere. The object written by RVA `0x059C40` has as its first word the client class table, image `0x2FFDC8`, so it is not a creature. None of the others was shown to be a creature.
- **Consequence.** On the client, these values change only through construction, the resets, copies, and received records. The creature at client `+0x8006D0` never takes them from a record. Whether the sender ever produces nonzero values is outside this image.
- **Lead.** The reconstruction names these "finalHandsRotation" and "initialHandsRotation".
- **Other readers** (besides the pose refresh): the record writer RVA `0x0CDAA0` (operands formed at `0x0CDDBB`, `0x0CDDC3`, `0x0CDDDB`, `0x0CDDE3`) and the copy routines.

## 5. Owner `+0x144`

- **Storage and record.** Binary32 (E`+0x134`), record bit 23, 4 bytes.
- **Initial value.** 0.
- **Client update RVA `0x088EE0`.** Applies to the creature at client `+0x8006D0`, when byte `+0x45` of the structure addressed by ESI is 1 (`0x08FF7A`). That structure's owner was not identified. In order:
  1. `+0x170` = `+0x174` + `+0x170` (`0x08FFD3`).
  2. `+0x144` = `+0x174` + `+0x144` (`0x08FFF1`).
  3. If `+0x170` > 1.0 (ordered comparison; image `0x345DC0`), `+0x170` = 1.0.
  4. If `+0x144` > `+0x170` (ordered), `+0x144` = `+0x170` (`0x090035`).
- **RVA `0x196D30`.** Applies to the creature in EDI, when the byte at frame `+0x3C` is nonzero, the predicate RVA `0x044590` on the creature is true, and `+0x174` > +0.0 (RVAs `0x19701B`–`0x197043`). It forms both sums from the values before either store (stores at `0x197087` and `0x19708F`), then applies the two clamps above (the second at `0x1970B0`).
- **World update RVA `0x20C510`.** All of the following act on the creature in frame slot `−0x2B14`:
  - Section 7 step 4 sets `+0x144` = 0 at `0x213608`.
  - At `0x215CD1`, `0x21714C` and `0x2176A9`, `+0x144` is set to 0 just after being copied into `+0x13B4` (Section 7).
  - At `0x218191` it is set to 0 after a `+0x170` update. The enclosing condition was not traced.
  - At `0x21A25C` it is set to 0 on the path that also sets `+0x68` = 0 when `+0x68` is not `0x05`, `0x14` or `0x11`. The test that selects this path was not traced.
  - At `0x21AE66` it is set to 0 after a call to RVA `0x02F160` on creature `+0x10`.
  - **Accumulation at `0x21A784`.** When `+0x68` = `0x24`: `+0x144` = (r × (L × 0.0005)) + `+0x144`. Here L is the frame local `−0x2B88`, 0.0005 is bits `0x3A03126F` (image `0x31E1DC`), and r is the result of RVA `0x047700` on the creature, stored as binary32.
  - **Accumulation at `0x21A8DE`.** `+0x144` = v + `+0x144`, where v is either the frame local `−0x2B20`, or ((s × 9) + 1.0) × that local, with s a preceding call's binary32 result and 9 from image `0x302A90`. The condition that selects between the two forms was not traced. Then, if RVA `0x043F10` with argument 10 returns nonzero, `+0x144` = `+0x170` (`0x21A8F5`). Finally, if `+0x144` > `+0x170` (ordered), `+0x144` = `+0x170` (`0x21A910`).
- **Readers** in listed code: `0x08FFE9`, `0x090028`, `0x197078`, `0x2135A2`, `0x2135BF`, `0x215CA0`, `0x215CC5`, `0x217140`, `0x21711A`, `0x217681`, `0x217695`, `0x21A77C`, `0x21A8D2`, `0x21A903`, plus the record and copy routines.
- **Lead.** The reconstruction calls this field "charge".

## 6. Owner `+0x140` and `+0x141`

- **Storage and record.** Two bytes (E`+0x130`, E`+0x131`), record bits 21 and 22.
- **Initial value.** 0 and 0.
- **World update (`0x210022`–`0x210043`).** A copy from a structure addressed by EDI into the creature in ESI: `+0x190` ← structure `+0x34`, `+0x60` ← `+0x28`, `+0x64` ← `+0x2C`, `+0x140` ← byte `+0x30`, `+0x141` ← byte `+0x31`. The structure was not identified; the reconstruction suggests a spawn entry (lead).
- **RVA `0x04BE40`.** Reads `+0x140` and then `+0x141` as consecutive bytes from a byte stream (`0x04C0C4`, `0x04C0E9`). This happens after Reset A (`0x04BE7F`) and the entity-block constructor (`0x04BF8A`), and before the copy at `0x04BF93`. Caller: `0x080863`.
- **RVA `0x02C080`.** On the creature at client `+0x8006D0`: `+0x64` = ECX (`0x02C129`); `+0x140` = (byte `+0x190` of the object in EDI) + 1, wrapping (`0x02C140`).
- **RVA `0x07B600` (at `0x07C893`).** On the creature at client `+0x8006D0`: `+0x141` = byte `+0x18C` of the object at client `+0x80090C`.
- **RVAs `0x083E70` (at `0x084074`) and `0x059C40` (at `0x062176`).** On the creature at client `+0x8006D0`: `+0x140` = 1, each preceded by `+0x60` = 0.
- **Unattributed.** RVA `0x1EE7A0` stores 16-bit `0x0102` or `0x0103` at `+0x140` of the object in EDI (`0x1EE951`, `0x1EE95C`). That object was not shown to be a creature.

## 7. Owner `+0x13B4`

- **Storage.** Binary32, client-local.
- **Initial value.** 0 at construction and after Resets A and B.
- **Main block (`0x2134F1`–`0x213610`).** It applies to the creature in slot `−0x2B14` and is reached after the store `+0x6C` = (converted value) at `0x2134EE`. The code that leads to `0x2134EE` was not traced.
  - If `+0x68` is `0x1F` or `0x21`, the block goes to `0x213754`, described below.
  - Otherwise, if `+0x6C` ≠ 0, it goes to `0x2137D9` (not traced).
  - Otherwise it runs these steps in order:
    1. Byte `+0x13C0` = 0, and `+0x13B4` = 0 (`0x213527`).
    2. m = RVA `0x044AE0`(`+0x68`, −1), stored as binary32.
       - If m > +0.0 (ordered): the call is repeated and its binary32 result r is stored in `+0x13B4` (`0x213577`); then `+0x170` = `+0x170` − r. Go to step 3.
       - Otherwise, if `+0x68` is `0x11`, `0x14` or `0x05`: `+0x13B4` = `+0x170` (word copy, `0x213683`), then `+0x170` = 0, then RVA `0x043F10`(11) is called. If it returns 0, go to step 3. Otherwise:
         - a record whose first byte is `0x0B` and whose word at `+8` is 0 is prepared (RVA `0x02C520`) and passed to RVA `0x046AF0` on the creature;
         - an object built by RVA `0x03B660` receives the creature's words `+0x8` and `+0xC` and that record, and is passed to RVA `0x0460A0` on the world's second argument `+0x58`;
         - byte `+0x13C0` = 1 (`0x213748`). Go to step 3.
       - Otherwise, if `+0x68` is `0x36`, `0x15`, `0x57` or `0x58`: `+0x13B4` = 1.0 (`0x21366E`). Go to step 3.
       - Otherwise `+0x13B4` stays 0. Go to step 3.
    3. (`0x213593`) If `+0x144` > +0.0 (ordered): `+0x13B4` = `+0x144` (`0x2135AF`), then `+0x170` = `+0x170` − `+0x144`, then byte `+0x13B8` = 0. If `+0x68` is `0x0B` or `0x05`, `+0x13B4` = `+0x13B4` × 0.5 (image `0x345DA0`, `0x2135F7`).
    4. `+0x144` = 0 (`0x213608`).
- **Path at `0x213754`** (`+0x68` = `0x1F` or `0x21`). B denotes RVA `0x03CAA0`(−1), evaluated afresh at each use. If `+0x6C` > B, go to step 3 above. Otherwise, if (`+0x6C` + L) > B, `+0x13B4` = 0.1 (bits `0x3DCCCCCD`, `0x213786`); the continuation was not traced. All comparisons are signed and the sum wraps. L is the world update's first stack argument, held in frame `−0x2B24` (`0x20C549`–`0x20C54F`).
- **Branch at `0x215CCB`.** It applies when the action value in CL is `0x68`, `+0x6C` = 0, and the world/creature gate holds (RVAs `0x215C68`–`0x215C96`). The world is the world update's receiver, kept in frame `−0x2B18`. The gate holds when the creature is the creature pointer at world `+0xB8`, or when world byte `+0xB4` is 0 and creature byte `+0x60` is nonzero. Then: `+0x170` = `+0x170` − `+0x144`; `+0x170` is set to 0 when the comparison register at `0x215CA8` is greater (that register's value was not traced); `+0x13B4` = `+0x144`; `+0x144` = 0.
- **Branch at `0x217146`.** It applies when the world/creature gate holds (RVAs `0x21707A`–`0x217095`), `+0x6C` ≤ B (`0x21709F`–`0x2170A7`) and (`+0x6C` + L) > B (to `0x2170C1`). The enclosing case selector was not identified. m = RVA `0x044AE0`(`+0x68`, −1). If m > +0.0 the routine is called again and `+0x170` = `+0x170` − result; otherwise `+0x170` = `+0x170` − `+0x144`. If 0 > `+0x170`, `+0x170` = 0. Then `+0x13B4` = `+0x144` and `+0x144` = 0.
- **Branch at `0x217687`.** It uses the same world/creature gate (RVAs `0x217634`–`0x21764F`) and requires `+0x6C` ≤ B and (`+0x6C` + L) > B. Then: `+0x13B4` = `+0x144`; `+0x170` = `+0x170` − `+0x144`; RVA `0x043F10`(10) is called; `+0x144` = 0. The enclosing case selector was not identified.
- **Branch at `0x21A104`.** It applies when `+0x68` = `0x30`, and either a byte register is nonzero or `+0x6C` > 500. Then:
  - `+0x13B4` = 0, `+0x170` = 1.0, `+0x68` = 0, and `+0x6C` = 0;
  - if `+0xAA8` = 3, `+0x68` is set from `+0xAA9`: 5 stays 5, 3 gives `0x11`, and any other value gives `0x14`;
  - then (`0x21A155`), if `+0x68` = `0x2F`: `+0x13B4` = 0 and `+0x68` = `0x36`.
- **Other readers** (the pose refresh reads it at `0x017FA2`): `0x2135E7`, `0x213625`, `0x214561`, `0x21473C`, `0x2148DB`, `0x2164CA`, `0x2165F9`, `0x21718E`, `0x21721F`, `0x2172D8`, `0x21746D`, `0x2178A2`, `0x217945`.
- **Lead.** The reconstruction calls this field "m_attackPower".

## 8. Owner `+0x1314`

- **Storage.** 32-bit integer, client-local. The pose selectors read it as signed.
- **Initial value.** 0 at construction and after Resets A and B.
- **Writers.** Both writers are in the world update, act on the creature in slot `−0x2B14`, and are reached from `0x212CB2` onward. The code leading to `0x212CB2` was not traced. B = RVA `0x03CAA0`(−1) and C = RVA `0x047310`(−1), each evaluated afresh at every use (both are pure; held helper-tables contract). L is as in Section 7. Integer sums and products wrap, division truncates toward zero, and comparisons are signed.
- **Writer 1 (`0x212E9B`).**
  - It requires `+0x68` ∈ {`0x48`, `0x56`, `0x1E`, `0x1F`, `0x20`, `0x21`, `0x0B`, `0x05`, `0x5B`}, B ≤ `+0x6C`, and `+0x6C` < B + C.
  - Period P: 250 by default; 1000 for `0x5B`; C ÷ 3 for `0x0B` and for `0x05`; C ÷ 6 for `0x1E` and `0x20`; C ÷ 12 for `0x1F` and `0x21` (`0x212D27`–`0x212DDB`).
  - K1 = (((`+0x6C` − B) ÷ P) × P) + B.
  - K2 = ((((`+0x6C` + L) − B) ÷ P) × P) + B.
  - When K2 ≠ K1 and K2 < B + C, the routine sets `+0x1314` = owner `+0x70` (32-bit copy).
  - K1 + 100 is also kept, in frame `−0x2B50`.
- **Writer 2 (`0x213077`).**
  - The code at `0x213052` reaches it in these cases: when `+0x68` is outside the set above (jump at `0x212CDE`); after the writer-1 block on its later paths (`0x212F56`, `0x212FD2`, `0x213035`, and `0x212EFC` when `+0x68` = 5); and from the entries at `0x213037` (when `+0x6C` < B) and `0x21303D` (when `+0x6C` ≥ B + C).
  - At those entries it evaluates B and, if `+0x6C` ≤ B, sets `+0x1314` = owner `+0x70`. Otherwise it continues at `0x213200` (not traced).
- **Random draws nearby.** Between the two writers, on the path from `0x212EC4` with `+0x68` ≠ 5, the routine calls import slot `0x2FC468` (`rand`) once at `0x212F7C`. It calls it a second time at `0x212FEF` when `+0x68` = `0x5B`. The path is taken when (`+0x6C` − L) < B, or when the two period quotients differ. The results feed RVA `0x0864C0` and do not write any field of this contract. They are part of the world update's draw order.
- **Readers in the pose refresh:** `0x01C222`, `0x01C650`, `0x01C78D`, `0x01DB8F`, `0x01E03E`. No other reader was found.
- **Lead.** The reconstruction calls this field "m_swingCount" and owner `+0x70` "hitCombo".

## 9. Owner `+0x58`, `+0x64`, `+0x7E`, the `+0x1E8` record, and the lean inputs

- **`+0x58`.** Binary32 (E`+0x48`), record bit 5, initial 0. The only writers attributable to a creature, in listed or decoded code, are record application and the entity-block copies. Decoded zero stores at displacement E`+0x48` (for example `0x18E9B9`, `0x193EAB`, `0x1AED69`, `0x2245CD`, `0x25AC06`, `0x272D8B`) were not shown to address creature blocks. For the creature at client `+0x8006D0`, which records never overwrite, no writer was found.
- **`+0x64`.** Signed 32-bit (E`+0x54`), record bit 8, initial 0. Writers attributable to a creature:
  - world update `0x210034` (from structure `+0x2C`, Section 6);
  - world update `0x20E862`: the zero-extended byte at `+0x1021` of the object addressed by the word at EDI, written just after `+0x60` = 5;
  - `0x02C129` (Section 6);
  - record application and the copies.

  The mount's `+0x64`, which selector `0x3A` reads, is the same field of the mount creature and has the same writers. Decoded stores at E`+0x54` (for example `0x067CF0`, `0x1AED44`, `0x2245A8`, `0x23C663`, `0x24D831`) were not attributed.
- **`+0x7E`.** 16-bit, inside record field E`+0x68` (bit 13), initial 0 (Section 3). Writers attributable to a creature:
  - world update `0x215A42`: after the creature-`+0x78` field is copied from one creature to another by RVA `0x059800`, the destination's `+0x7E` = (`+0x7E` AND `0xFDFF`) OR `0x0800`;
  - record application and the copies.

  The byte store at `0x2B7735` and the decoded 16-bit store at E`+0x6E` at `0x2BA747` were not attributed.
- **`+0x1E8` record.** `0x118` bytes at E`+0x1D8`, record bit 43. Bytes `+0x1E8` and `+0x1E9` start at 0, so the pose helper RVA `0x0C6B80` returns 0 for a new creature. Attributable writers are record application and the copies only. Candidate stores at `0x02CEC2` (16-bit `0x0401`), `0x2C7635`, `0x26524F` and others were not attributed. The world-update store at `0x210102` addresses creature `+0x1D8`, not this record. Lead: "actionItem".
- **Lean inputs** of the weapon contract (its Section 3): owner `+0x34`–`+0x3C` and `+0x40`–`+0x48` (bits 2 and 3, 12 bytes each), `+0x5C` (bit 6), `+0x68` (bit 9) and `+0x124` (bit 14). On creatures other than the one at client `+0x8006D0`, these fields are rewritten at every record application (Section 2). Their other writers were not enumerated. The blend repeat count, client `+0x8006E8`, is not an owner field.

## 10. Frame order

- Client update. The client update RVA `0x088EE0` (frame slot `0x28`) runs before the renderer (slot `0x2C`), which runs the pose refresh (held weapon contract, Section 12). It calls the world update at RVAs `0x08CF2E` and `0x08CF49`. Its own writes at RVAs `0x08FFF1`, `0x090035` and `0x097395` therefore also precede the frame's pose refresh. Their order relative to the world update calls was not established.
- Network receive path. Record application (Section 2) runs in a separate routine. Its scheduling relative to the frame was not established.

## 11. Models collection entries 840 and 2101

- The collection is the refresh's fourth argument.
- Lookup RVA `0x0120C0` returns the 32-bit word at index i of a contiguous array (begin at `+0x4`, end at `+0x8`), or null.
- How the array is filled and what entries 840 and 2101 hold were not traced.
- The reconstruction's identifier table has no entry with either number.
- No game content is reproduced here.

## 12. Random draws

- **Method.** The rand-calling routines were taken from `calls.csv`: 64 routines call `rand`, `srand`, or both.
- **Helpers called at the writer sites.** RVAs `0x044AE0`, `0x043F10`, `0x047700`, `0x044590`, `0x03CAA0`, `0x047310`, `0x059800` and `0x028750` were checked through their transitive direct-call closures in `calls.csv` (closure sizes 3, 1, 6, 1, 7, 7, 1 and 1). None of these closures reaches a rand-calling routine, and none contains a computed call.
- **Writer instructions.** None of the writer instructions above draws a random number.
- **Nearby draws.** The only draws found next to them are the two `rand` calls of Section 8, which do not write these fields.

## Unresolved

- Exhaustive writer lists for `+0x58`, `+0x64`, `+0x7E`, the `+0x1E8` record and the lean inputs. Several candidate stores at these displacements, both listed and decoded, were not attributed to any object.
- The code that leads to the world-update blocks at `0x212CB2` and `0x2134EE`. The continuations at `0x2137D9`, `0x213200` and after `0x213786`. The case selectors around `0x217146` and `0x217687`. The comparison value at `0x215CA8`. The conditions around `0x218191`, `0x21A25C` and `0x21A8A2`. The derivation of frame locals `−0x2B20` and `−0x2B88`.
- The roles of RVA `0x196D30`, of the structure tested at `0x08FF7A`, of the structure copied at `0x210022`–`0x210043`, and of RVA `0x1EE7A0`'s target.
- The order of the client update's own owner writes relative to its world-update calls, and the scheduling of the network receive routine relative to the frame. Whether the calls on client `+0x2E4` are locks.
- Whether the sender ever produces nonzero `+0x148`–`+0x15C` (outside this image).
- The identity and filling of models collection entries 840 and 2101.
- Readers listed only where scanned (`+0x144`, `+0x13B4`, `+0x1314`, `+0x148`/`+0x154`). No exhaustive reader list exists for the other fields.
