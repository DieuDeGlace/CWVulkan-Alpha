Status: FINAL / STABLE / HELD

# Alpha rulings for the creature delta codec group count and the dedicated-server join record

## Supported images

Original 32-bit Alpha client `binary/Cube.exe`, SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and
dedicated server `binary/Server.exe`, SHA-256
`1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`, both
with preferred image base `0x00400000`. Hashes were recomputed for this
ruling. Addresses are RVAs; server addresses are labelled. A **state
offset** is relative to creature `+0x10` (state `+0x050` is creature
`+0x60`). Evidence is the original executables, the original instruction
listings and the original exports under `disassembled alpha cube world`.
No reconstruction material was used.

## Ruling 1: the creature delta codec has exactly 48 groups

**Winner: `alpha-net-packets.md` §2.1 (48 fields, mask bits 0–47).** The
phrase "visit the 47 groups" in `alpha-network-creature-state-defaults.md`
(section "Unspecified state storage and conditional decode") is a
miscount; that contract's own outer table, its order statement and the
original images all give 48. Its order statement is otherwise confirmed.

Evidence of the count: the client state reader at `0x0CCFA0` makes exactly
48 calls, all of them group readers, and the client state writer at
`0x0CDAA0` makes 54 calls, namely one list-construction call, 48 group
writers (`0x0CDB0C` through `0x0CE0CB`) and five buffer/teardown calls.
The server writer at server RVA `0x016210` (48 group writers,
`0x01627C` through `0x01683B`) and the server reader at server RVA
`0x015DD0` (48 group readers, `0x015E1A` through `0x0161EF`) have the same
count, offsets, widths and order.

Mask consumption: both sides keep one group index that starts at 0 (the
reader clears it before its first call; the writer clears it before its
first group call). Every group routine, whether or not the group is
present or changed, advances the index by exactly one. Bit *n* lives in the
low 32-bit half of the mask for *n* below 32 and in the high half
otherwise. The writer sets bit *n* when the group differs from its
reference or when the force-all flag is nonzero, appends the group's new
bytes to a pending list, and after the 48th group emits the eight-byte mask
followed by the pending group bytes in visit order. Because only 48
indices are ever used, mask bits 48–63 are always zero and the force-all
mask is `0x0000FFFFFFFFFFFF`. The reader tests the same bit and, when set,
takes the group's bytes from the body in the same order. [Evidence:
representative writer `0x0CB710`, readers `0x0CBFC0` and `0x0CC5C0`;
writer mask emission after `0x0CE0CB`.]

Bit-to-group order (state offset, width in bytes, payload kind), shared by
writer and reader in both images:

| Bit | Offset | Bytes | Payload |
| --- | --- | --- | --- |
| 0 | `+0x000` | 24 | three 8-byte integers |
| 1 | `+0x018` | 12 | three binary32 |
| 2 | `+0x024` | 12 | three binary32 |
| 3 | `+0x030` | 12 | three binary32 |
| 4 | `+0x03C` | 12 | three binary32 |
| 5 | `+0x048` | 4 | binary32 |
| 6 | `+0x04C` | 4 | 4-byte integer |
| 7 | `+0x050` | 1 | byte |
| 8 | `+0x054` | 4 | 4-byte integer |
| 9 | `+0x058` | 1 | byte |
| 10 | `+0x05C` | 4 | 4-byte integer |
| 11 | `+0x060` | 4 | 4-byte integer |
| 12 | `+0x064` | 4 | 4-byte integer |
| 13 | `+0x068` | 172 | raw appearance block |
| 14 | `+0x114` | 2 | 2-byte integer |
| 15 | `+0x118` | 4 | 4-byte integer |
| 16 | `+0x11C` | 4 | 4-byte integer |
| 17 | `+0x120` | 4 | 4-byte integer |
| 18 | `+0x124` | 4 | 4-byte integer |
| 19 | `+0x128` | 4 | 4-byte integer |
| 20 | `+0x12C` | 4 | binary32 |
| 21 | `+0x130` | 1 | byte |
| 22 | `+0x131` | 1 | byte |
| 23 | `+0x134` | 4 | binary32 |
| 24 | `+0x138` | 12 | three binary32 |
| 25 | `+0x144` | 12 | three binary32 |
| 26 | `+0x150` | 12 | three binary32 |
| 27 | `+0x15C` | 4 | binary32 |
| 28 | `+0x160` | 4 | binary32 |
| 29 | `+0x164` | 4 | binary32 |
| 30 | `+0x168` | 20 | five binary32 |
| 31 | `+0x17C` | 1 | byte |
| 32 | `+0x17D` | 1 | byte |
| 33 | `+0x180` | 4 | 4-byte integer |
| 34 | `+0x184` | 4 | 4-byte integer |
| 35 | `+0x188` | 8 | 8-byte integer |
| 36 | `+0x190` | 8 | 8-byte integer |
| 37 | `+0x198` | 1 | byte |
| 38 | `+0x19C` | 4 | 4-byte integer |
| 39 | `+0x1A0` | 12 | three 4-byte integers |
| 40 | `+0x1B0` | 24 | three 8-byte integers |
| 41 | `+0x1CC` | 12 | three 4-byte integers |
| 42 | `+0x1C8` | 1 | byte |
| 43 | `+0x1D8` | 280 | one raw item |
| 44 | `+0x2F0` | 3640 | thirteen raw items |
| 45 | `+0x1158` | 16 | name storage |
| 46 | `+0x1128` | 44 | eleven 4-byte integers |
| 47 | `+0x1154` | 4 | 4-byte integer |

The order is ascending state offset except that bit 41 (`+0x1CC`) precedes
bit 42 (`+0x1C8`), and the last three groups are `+0x1158`, `+0x1128`,
`+0x1154`. Bits 0 and 40 use the same three-8-byte routine pair (client
writer `0x0CBC80`, reader `0x0CC5C0`). The present-group payload sizes sum
to the extent of the 48 groups only; the uncovered bytes `+0x051–+0x053`,
`+0x059–+0x05B`, `+0x116–+0x117`, `+0x132–+0x133`, `+0x17E–+0x17F`,
`+0x199–+0x19B`, `+0x1AC–+0x1AF` and `+0x1C9–+0x1CB` belong to no group.
[Evidence: `decompiled/client/functions/004ccfa0.c`, `004cdaa0.c`;
`decompiled/server/functions/00415dd0.c`, `00416210.c`; call lists in
`decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`.]

## Ruling 2: what the dedicated-server join writes

**Winner: `alpha-network-creature-state-defaults.md` §"Server player-join
overrides" for steps 1–6, which are confirmed store for store, extended
here by the closed effect of the routine at server RVA `0x00A840`.
`alpha-net-server.md` §3.1 is superseded where it differs:** its "all
other fields keep the record initialiser's defaults (U-4)" is wrong for
the thirteen-item block, and its collision box of 1.2, 1.2, 2.7 is not the
transmitted value. That box exists only transiently; the record the server
sends and keeps carries appearance `+0x08`, `+0x0C`, `+0x10` bits
`0x3F75C290`, `0x3F75C290`, `0x400A3D71` (approximately 0.96, 0.96,
2.16).

Exact sequence inside the join at server RVA `0x0254A0`, all under the
world lock taken at `0x02588D` and released at `0x025A9F`:

1. Construction at server RVA `0x006400` (`0x0258B9`) gives the defaults
   of `alpha-network-creature-state-defaults.md`, then the creature is
   inserted under its id (`0x0258EC`).
2. The reset at server RVA `0x0110D0` (`0x0258F5`) calls the state reset at
   server RVA `0x011360`, whose stores are exactly as listed in that
   contract's step 1. Against a just-constructed creature the only state
   changes it makes are in the thirteen-item block: one `memset` of the
   `0xE38` bytes at `+0x2F0` to zero (`0x011376–0x011432`). Each block
   item therefore ends with both selectors zero, level (item `+0x10`)
   **0** instead of 1, component count zero, and its five otherwise
   unspecified bytes zero. The standalone item at `+0x1D8` keeps level 1.
   The appearance is re-installed from a fresh stack appearance through
   the server appearance initializer `0x006970` and copier `0x007730`;
   appearance byte `+0x05` stays unwritten. Every other reset store
   rewrites a value equal to the construction default.
3. Position (`0x0258FA–0x02593C`): for components 0, 1, 2 in that order,
   the binary32 at world `+0x8000F0` plus four times the index is
   multiplied by `65536` (bits `0x47800000` at server RVA `0x15878C`) in
   single precision, then converted by server RVA `0x14A946` to an 8-byte
   integer stored at state `+0x000 + 8 × index`.
4. Collision box (`0x02593E–0x0259CB`): immediates `0x3F4CCCCD`,
   `0x3F4CCCCD`, `0x3FE66666` are each multiplied in single precision by
   `1.5` (bits `0x3FC00000` at server RVA `0x1586E8`). The products are
   not formed from the creature's existing appearance values (1.0). Under
   round-to-nearest-even they are `0x3F99999A`, `0x3F99999A` and
   `0x402CCCCC`; the third product is an exact halfway case that rounds to
   even, so it is not the binary32 nearest 2.7 (`0x402CCCCD`). They are
   stored to appearance `+0x08` and `+0x0C` as one 8-byte store, then
   `+0x10`.
5. Zeroes (`0x0259D4–0x025A03`): the triplet at state `+0x018` (an 8-byte
   then a 4-byte store), the triplet at `+0x024` (same form), then the
   4-byte value at `+0x04C`. All already held zero.
6. Level `+0x180` = `1` (`0x025A0A`), already the default.
7. The maximum-health query at server RVA `0x00FDA0` is called on the
   creature (`0x025A14`); then the controller byte at `+0x050` = `0x00`
   (`0x025A19`); then the query result is stored as binary32 health at
   `+0x15C` (`0x025A1D`). The query sees controller byte `3`, class
   `+0x130` = 0, type `+0x054` = 0 and the zeroed thirteen-item block.
8. Type `+0x054` = `0x2C` (`0x025A23`).
9. The six 4-byte words of state `+0x000–+0x017` are copied in ascending
   order to `+0x1B0–+0x1C7` (`0x025A2A–0x025A5D`).
10. Items (`0x025A6A–0x025A81`), in this order: block item `+0xA98` byte
    `+0x0D` = `0x01`; its selectors `+0x00`, `+0x01` = `0x03`, `0x02` (one
    2-byte store of `0x0203`); block item `+0x980` byte `+0x0D` = `0x01`;
    its selectors = `0x03`, `0x02`. These two items keep level 0.
11. Class `+0x130` = `0x01` (`0x025A8A`); `+0x131` stays 0.
12. The appearance generator at server RVA `0x00A840` is called
    (`0x025A91`, caller pops 12 bytes) with the address of state `+0x054`,
    the address of the appearance block and a null third argument.

Generator effect for this call, from server RVA `0x00A840–0x00A9E2` and
the type-44 target `0x00AC35`:

* Because the third argument is null, it makes **exactly five** calls to
  the imported CRT `rand` (import slot `0x5582F4`), in this order: draw 1
  reduced to a signed remainder modulo 256, draw 2 likewise, draw 3
  likewise, draw 4 signed remainder modulo 100, draw 5 signed remainder
  modulo 100. The results go only to the generator's own stack descriptor;
  the type at `+0x054` is not changed on this path.
* It then unconditionally stores appearance `+0x08` = `0x3F75C290`,
  `+0x0C` = `0x3F75C290`, `+0x10` = `0x400A3D71`, replacing step 4's
  products.
* It dispatches on the type through the table at server VA `0x0040ED20`
  (bound `0x9B`). Type 44 goes to `0x00AC35`, which stores 2-byte
  appearance values `+0x14` = `0x000A`, `+0x16` = `0xFFFF`, one 4-byte
  store of `0x000001B0` at `+0x1A` (so `+0x1A` = `0x01B0`, `+0x1C` =
  `0x0000`), then `+0x18` = `0x01AE` (`0x00AAA8`), and returns. None of
  the five draws is consumed by this case.

After the lock is released the join fills the inventory at creature
`+0x11DC` (fifty calls of server RVA `0x027000`, then one more), outside
the codec state, and sends the handshake: a 4-byte `0`, the 8-byte id and
the full `0x1168` bytes from creature `+0x10` (`0x025CF7–0x025D4F`).
No instruction between step 12 and that send writes the codec state.

Resulting join record, relative to the construction defaults: position and
its copy at `+0x1B0` as in steps 3 and 9; controller byte 0; type 44;
health from the query; class 1; appearance as constructed except `+0x08`,
`+0x0C`, `+0x10` (bits `0x3F75C290`, `0x3F75C290`, `0x400A3D71`), and
`+0x14`, `+0x16`, `+0x18`, `+0x1A`, `+0x1C` (`0x000A`, `0xFFFF`,
`0x01AE`, `0x01B0`, `0x0000`); thirteen-item block all zero except the
selector and `+0x0D` bytes of items `+0x980` and `+0xA98`; every other
group equal to its construction default. The final height (appearance
`+0x10`) is bits `0x400A3D71`. The 1.5 products of step 4 are dead stores
for type 44.

## Residual uncertainty

* The values of the five draws depend on the CRT `rand` state of the
  calling accept thread; whether that state is per thread, and its seed
  and prior consumption, are external MSVCR110 behavior not closed here.
* The rounding of the conversion at server RVA `0x14A946`, the values at
  world `+0x8000F0`, and the maximum-health result are not closed.
* The single-precision products assume the default round-to-nearest-even
  control state; no instruction in the traced path changes it.
* Between the unlock at `0x025A9F` and the handshake send, other threads
  may hold the world lock; whether they can change this creature before
  the send is not closed.
* Gameplay meanings of the appearance values and of the item selectors
  are not closed. Truncated or malformed state bodies remain outside
  Ruling 1.
