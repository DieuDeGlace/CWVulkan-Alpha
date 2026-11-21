# Alpha network creature-state defaults

**FINAL / STABLE / HELD**

Status: re-certified 2026-11-21 against the original client and dedicated-server images. This original-only contract establishes the state supplied by a newly constructed creature to the client packet-kind-0 receive path, and the equivalent construction in the dedicated server. It describes initialized properties and explicitly unspecified storage. It does not define a zero-filled creature image, a complete receive protocol, or the complete player-join state.

The primary supported image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, under the evidence root `disassembled alpha cube world`. The dedicated-server image is `binary/Server.exe`, SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both have preferred image base `0x00400000`. Addresses below are RVAs; an address is a client RVA unless it is explicitly labelled as a server RVA. Evidence paths are relative to the evidence root. All findings were independently checked in original exports and, for the construction, copy and join boundaries, the original executable bytes. No prior contract or replacement implementation supplies these facts.

## Receive boundary and address conventions

Packet kind 0 dispatches to RVA `0x06BAAD`: the dispatcher bounds the kind to at most `0x0F`, maps it through the byte table at `0x06D1A4` (kind 0 maps to index 0), and jumps through the pointer table at `0x06D180`. After receiving and processing its body buffer, the path clears an eight-byte local ID, reads an eight-byte creature ID into it from the body, locks the creature collection, and looks up that ID. When the lookup returns no creature, it requests `0x1E60` bytes, calls the creature constructor at RVA `0x03B690` with the received ID, and inserts the resulting pointer under that ID. Existing creatures bypass this construction. The allocation call itself does not establish any initial value for its storage. [Evidence: `binary/Cube.exe`, dispatch at `0x06B8A7–0x06B8BD`, tables at `0x06D180` and `0x06D1A4`, ID read `0x06BBB5–0x06BBD3`, kind-0 branch `0x06BAAD–0x06BC4B`, allocation and constructor call `0x06BC02–0x06BC2D`; `decompiled/client/functions/0046b740.c`, RVA `0x06B740`, whose exported body does not cover the whole native switch.]

At RVA `0x06BC52`, both newly constructed and existing creatures supply the address at creature `+0x10`. The property copier at RVA `0x066050` copies that state into a stack-resident temporary. The receive path unlocks the collection at `0x06BC6F`, then calls the state reader at RVA `0x0CCFA0` on that temporary. Thus the defaults below are the temporary's starting properties when a creature was newly constructed. They are not reapplied to an existing creature on every packet. They also do not describe the final decoded state when the packet supplies replacements. [Evidence: `binary/Cube.exe`, RVA `0x06BC52–0x06BC87`; `decompiled/client/functions/00466050.c`, RVA `0x066050`; `decompiled/client/functions/004ccfa0.c`, RVA `0x0CCFA0`.]

Throughout this contract, a **state offset** is relative to creature `+0x10`. For example, state `+0x15C` is creature `+0x16C`. The copied state extends through state `+0x1167`, giving an extent of `0x1168` bytes. The creature's own eight-byte ID is at creature `+0x08`, before this state; creature `+0x00` holds a fixed four-byte type-table pointer (client VA `0x006FD8CC`, server VA `0x0055887C`) that is not codec state. The constructor copies the received ID there as two four-byte halves; it does not substitute that ID for either of the separate eight-byte properties within the codec state. [Evidence: `binary/Cube.exe`, RVA `0x03B6BD–0x03B6D1`; `decompiled/client/functions/00466050.c`, RVA `0x066050`, last copies at `0x0663DE–0x0663F6`; `binary/Server.exe`, server RVA `0x00642D–0x00643E`.]

## Initialized outer state

The creature constructor invokes the state initializer at RVA `0x03C100` on creature `+0x10`. Later successful constructor work concerns storage beginning at creature `+0x1178` and above; the inspected game-owned callees do not replace the initialized codec state. All binary32 zero values below have bits `0x00000000`, and all binary32 one values have bits `0x3F800000`. Values are direct fixed-width stores, without a maximum-health query, stat calculation, numeric conversion, or random selection. [Evidence: `binary/Cube.exe`, RVA `0x03B690–0x03BBF6` and `0x03C100–0x03C379`; `decompiled/client/functions/0043b690.c`; `decompiled/client/functions/0043c100.c`.]

The table covers the outer properties selected by the codec writer at RVA `0x0CDAA0`. Where this bounded evidence does not close a property's gameplay name, its address and width identify it without assigning an invented meaning. Rows containing multiple offsets describe separate properties at each listed offset, rather than a wider zero-filled allocation.

| State offset | Property width or grouping | Newly constructed value |
| --- | --- | --- |
| `+0x000` | Three eight-byte position components | All three zero |
| `+0x018`, `+0x024`, `+0x030`, `+0x03C` | Four separate triplets of binary32 components | Every component positive zero |
| `+0x048` | Binary32 scalar | Positive zero |
| `+0x04C` | Four-byte property | Zero |
| `+0x050` | One-byte controller value (see below) | `3`, bits `0x03` |
| `+0x054` | Four-byte creature-type property | Zero |
| `+0x058` | One-byte action value | Zero |
| `+0x05C`, `+0x060`, `+0x064` | Three four-byte properties | Zero each |
| `+0x068` | `0xAC`-byte appearance block | Fixed appearance properties listed below; one byte unspecified |
| `+0x114` | Two-byte property | Zero |
| `+0x118` | Four-byte property | Zero |
| `+0x11C` | Signed four-byte property | `-3000`, bits `0xFFFFF448` |
| `+0x120`, `+0x124`, `+0x128` | Three four-byte properties | Zero each |
| `+0x12C` | Binary32 property | Positive zero |
| `+0x130`, `+0x131` | Two separate one-byte class/specialization values | Zero each; the initializer writes them together as a two-byte zero |
| `+0x134` | Binary32 property | Positive zero |
| `+0x138`, `+0x144`, `+0x150` | Three separate triplets of binary32 components | Every component positive zero |
| `+0x15C` | Binary32 health at creature `+0x16C` | `500`, bits `0x43FA0000` |
| `+0x160`, `+0x164` | Two binary32 resource properties | Positive zero each |
| `+0x168` | Group of five binary32 properties | In address order: `100`, `1`, `1`, `1`, `1`; first bits `0x42C80000` |
| `+0x17C`, `+0x17D` | Two separate one-byte properties | Zero each; written together as a two-byte zero |
| `+0x180` | Four-byte level | `1` |
| `+0x184` | Four-byte property | Zero |
| `+0x188`, `+0x190` | Two separate eight-byte properties | Zero each |
| `+0x198` | One-byte rank property | Zero |
| `+0x19C` | Signed four-byte property | `-1`, bits `0xFFFFFFFF` |
| `+0x1A0` | Three four-byte words | In address order: `0xFFFFFFFF`, `0xFFFFFFFF`, `0x00000000` |
| `+0x1B0` | Three eight-byte properties | **Not initialized by the constructor chain**; the copier still reads all 24 bytes |
| `+0x1C8` | One-byte property | Zero |
| `+0x1CC` | Three four-byte words | In address order: `0xFFFFFFFF`, `0xFFFFFFFF`, `0x00000000` |
| `+0x1D8` | One `0x118`-byte item | Item properties listed below; padding unspecified |
| `+0x2F0` | Thirteen consecutive `0x118`-byte items | Each has the same item-property defaults below |
| `+0x1128` | Eleven four-byte properties, `0x2C` bytes total | Zero each |
| `+0x1154` | Four-byte property | Zero |
| `+0x1158` | Sixteen-byte name storage | All sixteen bytes zero, including an initial string terminator |

[Evidence for outer stores: `decompiled/client/functions/0043c100.c`, RVA `0x03C100`; `binary/Cube.exe`, RVA `0x03C103–0x03C36E`. Evidence for selected groups and widths: `decompiled/client/functions/004cdaa0.c`, RVA `0x0CDAA0`; scalar writers `004cb5d0.c`, `004cb670.c`, `004cb710.c`, and `004cb7b0.c`, RVAs `0x0CB5D0`, `0x0CB670`, `0x0CB710`, and `0x0CB7B0`; grouped writers `004cb850.c`, `004cbb10.c`, `004cbbc0.c`, `004cbc80.c`, and `004cbd40.c`, RVAs `0x0CB850`, `0x0CBB10`, `0x0CBBC0`, `0x0CBC80`, and `0x0CBD40`; name and eleven-word writers `004ccc30.c` and `004cccf0.c`, RVAs `0x0CCC30` and `0x0CCCF0`.]

The two words of `0xFFFFFFFF` in each three-word group are separate four-byte initializations. This contract does not reinterpret either group as one eight-byte creature ID. Likewise, the five-property block beginning at `+0x168` is five binary32 values, not binary64 values merely because the copier transfers parts of it eight bytes at a time. The health value of 500 is a constructor value, not a claim that it equals the creature's computed maximum. [Evidence: `binary/Cube.exe`, RVA `0x03C244–0x03C2EB` and `0x06620E–0x066234`; `decompiled/client/functions/00444db0.c`, RVA `0x044DB0`, is a separate maximum-health query absent from construction.]

### Default controller byte

State `+0x050` (creature `+0x60`) is a single byte. Both the client and the dedicated-server state initializers store exactly `0x03` there with a one-byte store; neither the constructor's later work nor the client receive-path copy changes it, so a newly constructed creature's temporary starts with `3`. Its three neighbouring bytes `+0x051–+0x053` are not written. The codec selects it as a one-byte group, and the copier transfers one byte. The original images treat zero specially: the client maximum-health query at RVA `0x044DB0` and its server counterpart at server RVA `0x00FDA0` take a separate branch when this byte is zero, and the server player-join path assigns zero (see below). The complete set of meaningful values and their gameplay names is not closed here. [Evidence: `binary/Cube.exe`, RVA `0x03C191` and copy at `0x0660D0–0x0660D4`; `binary/Server.exe`, server RVA `0x0070B1`; `decompiled/client/functions/00444db0.c`; `decompiled/server/functions/0040fda0.c`; `binary/Server.exe`, server join store at server RVA `0x025A19` and zero comparison at server RVA `0x02561A`.]

## Appearance properties

The appearance initializer at RVA `0x028750` receives state `+0x068`. The following offsets are relative to that appearance block, not the creature or state base. They establish its fixed property defaults; anatomical names and interpretation of individual appearance identifiers are not closed here.

| Appearance offset | Width | Initial value |
| --- | --- | --- |
| `+0x00`, `+0x01` | One byte each | `0x00` each |
| `+0x02`, `+0x03`, `+0x04` | One byte each | `0xFF` each |
| `+0x05` | One byte | Unspecified: neither initialized nor copied by the appearance property copier |
| `+0x06` | Two bytes | Zero |
| `+0x08`, `+0x0C`, `+0x10` | Binary32 each | `1` each |
| `+0x14`, `+0x16`, `+0x18`, `+0x1A`, `+0x1C`, `+0x1E`, `+0x20`, `+0x22` | Two bytes each | `0xFFFF` each |
| `+0x24` | Binary32 | Bits `0x3F8147AE`, approximately `1.01` |
| `+0x28`, `+0x2C` | Binary32 each | `1` each |
| `+0x30` | Binary32 | Bits `0x3F7AE148`, approximately `0.98` |
| `+0x34` | Binary32 | `1` |
| `+0x38` | Binary32 | Bits `0x3F733333`, approximately `0.95` |
| `+0x3C` | Binary32 | Bits `0x3F4CCCCD`, approximately `0.8` |
| `+0x40`, `+0x44` | Binary32 each | `1` each |
| `+0x48`, `+0x4C`, `+0x50`, `+0x54`, `+0x58`, `+0x5C`, `+0x60`, `+0x64`, `+0x68` | Binary32 each | Positive zero each |
| `+0x6C` | Binary32 | `-5`, bits `0xC0A00000` |
| `+0x70` | Binary32 | Positive zero |
| `+0x74` | Binary32 | `0.5`, bits `0x3F000000` |
| `+0x78` | Binary32 | `5`, bits `0x40A00000` |
| `+0x7C` | Binary32 | `6`, bits `0x40C00000` |
| `+0x80`, `+0x84` | Binary32 each | Positive zero each |
| `+0x88` | Binary32 | `3`, bits `0x40400000` |
| `+0x8C` | Binary32 | `1` |
| `+0x90` | Binary32 | `-10.5`, bits `0xC1280000` |
| `+0x94` | Binary32 | Positive zero |
| `+0x98` | Binary32 | `-8`, bits `0xC1000000` |
| `+0x9C` | Binary32 | `2`, bits `0x40000000` |
| `+0xA0`, `+0xA4`, `+0xA8` | Binary32 each | Positive zero each |

[Evidence: `decompiled/client/functions/00428750.c`, RVA `0x028750`; `binary/Cube.exe`, RVA `0x028750–0x0288A9`; appearance copier `decompiled/client/functions/00459800.c`, RVA `0x059800`.]

The first four appearance bytes are established by one four-byte store of bits `0xFFFF0000`, which in little-endian storage gives bytes `0x00`, `0x00`, `0xFF`, `0xFF` at `+0x00` through `+0x03`; byte `+0x04` is a separate one-byte store. The decimal approximations do not replace the listed binary32 bit patterns. The initializer stores those patterns directly and makes no random draws or arithmetic calculations for them. The appearance property copier transfers every listed initialized property but skips byte `+0x05`. The codec appearance group can nevertheless transmit or receive all `0xAC` bytes as a raw block. Its full raw payload therefore includes an unspecified byte when originating from this default construction/copy chain. [Evidence: `binary/Cube.exe`, RVA `0x028750–0x02875A`; appearance initializer and copier above; `decompiled/client/functions/004cb930.c`, RVA `0x0CB930`; `decompiled/client/functions/004cc1c0.c`, RVA `0x0CC1C0`.]

## Item properties and item order

The standalone item starts at state `+0x1D8`. The thirteen-item block starts at state `+0x2F0`, spans `0xE38` bytes, and ends immediately before state `+0x1128`. Its item bases in ascending storage order are `+0x2F0`, `+0x408`, `+0x520`, `+0x638`, `+0x750`, `+0x868`, `+0x980`, `+0xA98`, `+0xBB0`, `+0xCC8`, `+0xDE0`, `+0xEF8`, and `+0x1010`. This contract does not assign inventory-slot or equipment-slot names to those positions. [Evidence: `decompiled/client/functions/0043c100.c`, RVA `0x03C100`; thirteen-item initializer `0043bc00.c`, RVA `0x03BC00`; thirteen-item copier `00459b10.c`, RVA `0x059B10`; codec item-block writer `004cb9d0.c`, RVA `0x0CB9D0`.]

Each of these fourteen items has the same initialized properties. The offsets below are relative to the individual item.

| Item offset | Width or grouping | Initial value |
| --- | --- | --- |
| `+0x00`, `+0x01` | Two separate one-byte item selectors | Zero each; the initializer writes them together as a two-byte zero |
| `+0x04`, `+0x08` | Two four-byte properties | Zero each |
| `+0x0C`, `+0x0D`, `+0x0E` | Three one-byte properties | Zero each; the first two are initialized together |
| `+0x10` | Two-byte item level | `1` |
| `+0x14` through `+0x113` | 256-byte component storage | Every byte zero |
| `+0x114` | Four-byte component count | Zero |
| `+0x02–+0x03`, `+0x0F`, `+0x12–+0x13` | Five bytes outside the copied item properties | Unspecified |

[Evidence: `binary/Cube.exe`, standalone stores at `0x03C2EF–0x03C333`, first repeated item at `0x03BC00–0x03BC3F`; `decompiled/client/functions/0043bc00.c`, RVA `0x03BC00`, all thirteen items; item copier `0040ee70.c`, RVA `0x00EE70`; item equality helper `0042f4a0.c`, RVA `0x02F4A0`.]

The zero component count is distinct from the initialized component storage: the constructor clears all 256 component bytes even though the count is zero. For every item, the count store precedes that item's 256-byte clear. The item copier copies all that storage (32 passes of eight bytes each, covering every byte) and the count, together with the other listed item properties. It leaves the five listed bytes untouched in its destination. No item seed is randomly generated, no component is selected, and no category is inferred from allocation. The whole-item and thirteen-item codec groups use raw payload extents of `0x118` and `0xE38` bytes, respectively; these raw extents include the unspecified item bytes. Their equality helpers do not establish a zero value for those omitted-copy bytes. [Evidence: `decompiled/client/functions/0040ee70.c`, RVA `0x00EE70`, byte loop `0x00EED0–0x00EEF7`; `0042f4a0.c`, RVA `0x02F4A0`; `004cba70.c`, RVA `0x0CBA70`; `004cb9d0.c`, RVA `0x0CB9D0`; `004ccaa0.c`, RVA `0x0CCAA0`; item reader `004cc3c0.c`, RVA `0x0CC3C0`.]

## Unspecified state storage and conditional decode

Apart from the per-item gaps just listed, the successful constructor chain leaves these state bytes unwritten: `+0x051–+0x053`, `+0x059–+0x05B`, appearance byte `+0x06D`, `+0x116–+0x117`, `+0x132–+0x133`, `+0x17E–+0x17F`, `+0x199–+0x19B`, `+0x1AC–+0x1C7`, and `+0x1C9–+0x1CB`. No zero-fill of the `0x1E60`-byte creature allocation occurs in the inspected path. [Evidence: `binary/Cube.exe`, RVA `0x06BC02–0x06BC26`, `0x03B690–0x03BBF6`, `0x03C100–0x03C379`, and `0x028750–0x0288A9`; `decompiled/client/functions/0043bc00.c`.]

Most of these gaps are also skipped by the state property copier. State `+0x1B0–+0x1C7` is the exception: all 24 bytes are copied as six four-byte words despite having no constructor assignment. The four bytes at `+0x1AC–+0x1AF` are not copied and are not a writer-selected property. The codec explicitly recognizes the 24-byte group beginning at `+0x1B0`, using the same three-eight-byte-value writer and reader that serve the position at state `+0x000`. Its initial contents therefore remain unspecified data at a real codec boundary, rather than being dismissed as padding. [Evidence: `binary/Cube.exe`, RVA `0x0662D9–0x06631B`; `decompiled/client/functions/004cdaa0.c`, RVA `0x0CDAA0`, groups at state `+0x000` and `+0x1B0`; writer `004cbc80.c`, RVA `0x0CBC80`; reader `004cc5c0.c`, RVA `0x0CC5C0`.]

The temporary state is populated by property copies, not by copying the entire `0x1168`-byte extent or by clearing that extent first. Consequently, an omitted-copy padding byte in the temporary need not even equal the corresponding unspecified byte in the newly allocated creature. No reproducible value, cross-allocation equality, previous-stack value, or allocator pattern is specified for any unwritten byte. [Evidence: `binary/Cube.exe`, receive-worker prologue `0x06B740–0x06B8B7` and copy call `0x06BC52–0x06BC62`; complete state copier `0x066050–0x066403`; nested property copiers cited above.]

The state reader uses the packet's 64-bit presence mask to decide which properties to replace. Writer and reader visit the 47 groups in one shared order, and each visited group consumes the next mask bit, starting at bit 0 for the position. That order follows ascending state offset except that the `+0x1CC` group precedes the `+0x1C8` byte, and the trailing groups are the name storage at `+0x1158`, then the eleven words at `+0x1128`, then `+0x1154`; it therefore differs from the copier's order. An absent group advances the bit position without assigning that group, retaining the seeded value, including an unspecified value where the seed was unspecified. Present raw appearance or item groups can replace their full extents, including padding. This is why initialized semantic defaults, copy behavior, and raw serialized bytes are separate facts. This contract does not close malformed or truncated state-body handling, final application after decode, or remote interpolation. [Evidence: `decompiled/client/functions/004ccfa0.c`, RVA `0x0CCFA0`; `decompiled/client/functions/004cdaa0.c`, RVA `0x0CDAA0`; representative conditional readers `004cc5c0.c`, `004cc1c0.c`, and `004cc3c0.c`, RVAs `0x0CC5C0`, `0x0CC1C0`, and `0x0CC3C0`.]

## Construction order, random draws, and dependency limits

The successful relevant call order is fixed. The creature constructor first stores its fixed type-table pointer at creature `+0x00`, then copies the received creature ID to creature `+0x08`, then invokes the state initializer. That initializer establishes the leading scalar/vector properties and calls the appearance initializer once. It then establishes the middle scalar properties and the standalone item's properties, clears that item's component storage, and calls the thirteen-item initializer. The thirteen-item initializer visits ascending item storage order and clears each item's component storage before proceeding to the next. On returning, the state initializer clears the final four-byte property at `+0x1154`, clears both eight-byte portions of the name storage, and clears the eleven-word block at `+0x1128`. Within these phases, individual store order (for example, `-3000` at `+0x11C` after the neighbouring zeros) has no observable effect because every target is distinct. [Evidence: `binary/Cube.exe`, RVA `0x03B6BD–0x03B6D1` and `0x03C100–0x03C379`; `decompiled/client/functions/0043bc00.c`, RVA `0x03BC00`.]

The outer constructor then creates its empty resident/container state and initializes its other runtime members. Its successful game-owned helper order after state initialization is RVA `0x039600`; `0x03C870` twice; `0x03C020`; `0x03C8A0`; `0x03C870`; `0x25AE80` twice; `0x2391E0`; `0x06D4B0`; `0x03C8E0`; `0x278520`; `0x03C8E0`; and `0x011E50`, followed by the final component-storage clear outside the codec state. These helpers establish empty sentinels or fixed runtime-member values. Their allocated addresses and non-state storage are not codec defaults. [Evidence: `decompiled/client/functions/0043b690.c`, RVA `0x03B690`; original helper exports `00439600.c`, `0043c870.c`, `0043c020.c`, `0043c8a0.c`, `0065ae80.c`, `006391e0.c`, `0046d4b0.c`, `0043c8e0.c`, `00678520.c`, and `00411e50.c`; `binary/Cube.exe`, RVA `0x03B6D6–0x03BBDC`.]

There are **zero game random draws** in this traced successful construction and state-copy chain. The state, appearance, item, and property-copy routines make fixed stores or copies. The additional constructor helpers inspected above allocate/link empty sentinels or write fixed members; their only outgoing calls are the imported allocation, clear and allocation-failure routines, and none calls the game random generator or the imported random function. The copy order is outer state properties, appearance properties, later outer properties, standalone item, thirteen items in ascending storage order, the eleven-word block, the final four-byte property, and the name storage. This no-draw finding is limited to the traced chain; it does not define unrelated threads' random consumption or allocator internals. [Evidence: original routines cited in this section; state copier `decompiled/client/functions/00466050.c`, RVA `0x066050`; nested copiers `00459800.c`, `0040ee70.c`, and `00459b10.c`; `decompiled/client/calls.csv`, outgoing calls from the named constructor and initialization routines.]

Allocation success, imported allocation/clear/copy implementations, exception behavior, and the lifetime or concurrent mutation of the surrounding creature collection remain external dependencies. The allocation-failure path is not a source of valid default state: on a null allocation the native branch substitutes a null pointer, inserts it under the received ID, and then forms the state address from it; this contract supplies no invented recovery behavior. No imported power or other stat arithmetic is called to establish these values. Meanings of unnamed properties, anatomy identifiers, equipment-slot names, and all post-decode gameplay behavior remain explicitly unclosed. [Evidence: `binary/Cube.exe`, RVA `0x06BC19–0x06BC52`.]

## Dedicated-server construction

The dedicated server allocates `0x1E60`-byte creatures and constructs them with its creature constructor at server RVA `0x006400`. Its construction is store-for-store equivalent to the client's: after the type-table pointer and the two ID halves, it calls its state initializer at server RVA `0x007020` on creature `+0x10`. That initializer, its appearance initializer at server RVA `0x006970`, and its thirteen-item initializer at server RVA `0x006AD0` perform the same stores, with the same offsets, widths, bit patterns and order, as the client routines at `0x03C100`, `0x028750` and `0x03BC00`. In particular the server writes the controller byte at state `+0x050` as `0x03`, health `0x43FA0000`, `-3000` at `+0x11C`, and leaves the same state bytes unwritten, including `+0x1AC–+0x1C7`. Every table and gap statement above therefore also describes a freshly constructed server creature. [Evidence: `binary/Server.exe`, server RVA `0x006400–0x006968`, `0x007020–0x007299`, `0x006970–0x006AC9`, `0x006AD0–0x006EED`; `decompiled/server/assembly.txt`, instruction-by-instruction comparison with the client routines; `decompiled/server/functions/00407020.c`.]

The server constructor's helper order after state initialization corresponds one-to-one with the client's: server RVA `0x0F7B60`; `0x007B30` twice; `0x006EF0`; `0x007B60`; `0x007B30`; `0x007B00` twice; `0x007AD0`; `0x02F040`; `0x007BE0`; `0x12ED30`; `0x007BE0`; and `0x001140`, followed by the final non-state component-storage clear. Their only outgoing calls are the imported allocation, clear and allocation-failure routines, so construction makes zero random draws on the server as well. [Evidence: `decompiled/server/calls.csv`, outgoing calls from server RVAs `0x006400`, `0x007020`, `0x006970`, `0x006AD0` and the listed helpers.]

### Server player-join overrides (not construction defaults)

When a client joins, the server join path at server RVA `0x0254A0` constructs a creature for the connection, inserts it, and then immediately changes its state. These later values are join-time assignments, not creature defaults, and this contract does not close the complete join state. The observed sequence is:

1. The reset at server RVA `0x0110D0` calls the state reset at server RVA `0x011360` on creature `+0x10`. That reset stores: level `+0x180` = `1`; `+0x184` = zero; the binary32 triplets at `+0x024`, `+0x030`, `+0x03C` = zero (the first triplet at `+0x018` is not written); `+0x168` = `0x42C80000`, `+0x16C` and `+0x170` = `0x3F800000` (`+0x174` and `+0x178` are not written); `+0x058` = zero byte; `+0x05C`, `+0x060`, `+0x064` = zero; `+0x11C` = `0xFFFFF448`; `+0x120` through `+0x12F` = zero; the entire `0xE38`-byte thirteen-item block at `+0x2F0` = zero bytes (so every block item's level becomes `0` and its padding becomes zero); `+0x114`, `+0x17C`, `+0x130` = two-byte zeros; `+0x134`, `+0x160`, `+0x164` = zero; `+0x188` through `+0x197` = zero; `+0x198` = zero byte; the triplets at `+0x138` and `+0x144` = zero (the `+0x150` triplet is not written); `+0x1A0`/`+0x1A4`/`+0x1A8` and `+0x1CC`/`+0x1D0`/`+0x1D4` = `0xFFFFFFFF`, `0xFFFFFFFF`, `0x00000000`; `+0x1C8` = zero byte; the appearance block is replaced, through the appearance property copier at server RVA `0x007730`, by a freshly initialized stack appearance (byte `+0x05` again untouched); the name storage, the eleven-word block and `+0x1154` = zero. It does not write the controller byte, the position, `+0x048`, `+0x04C`, `+0x054`, `+0x118`, `+0x15C`, `+0x19C`, `+0x1B0` group, or the standalone item. The rest of `0x0110D0` concerns creature storage at `+0x1178` and above. [Evidence: `binary/Server.exe`, server RVA `0x0110D0–0x01135E` and `0x011360–0x011537`.]
2. The three position components at state `+0x000` are assigned by converting three binary32 values, each multiplied by `65536` (bits `0x47800000`), to 64-bit integers through the routine at server RVA `0x14A946`; the source of the three values and the conversion's rounding are not closed here. [Evidence: server RVA `0x0258FA–0x02593C`; constant at server RVA `0x15878C`.]
3. Appearance `+0x08`, `+0x0C`, `+0x10` (state `+0x070`, `+0x074`, `+0x078`) receive binary32 products of `0x3F4CCCCD`, `0x3F4CCCCD` and `0x3FE66666` with the constant `1.5` (bits `0x3FC00000`), giving bits `0x3F99999A`, `0x3F99999A` and `0x402CCCCC` under round-to-nearest single-precision multiplication. The binary32 triplets at state `+0x018` and `+0x024` are stored as zero; state `+0x04C` is zero; level `+0x180` is again `1`. [Evidence: server RVA `0x02593E–0x025A0A`; constant at server RVA `0x1586E8`.]
4. The server maximum-health query at server RVA `0x00FDA0` is evaluated, then the controller byte at state `+0x050` is set to `0x00`, then health at `+0x15C` receives the query result. The query therefore runs while the controller byte still holds the construction value `3` and while state `+0x130` still holds zero. [Evidence: server RVA `0x025A14–0x025A1D`.]
5. State `+0x054` is set to `0x2C` (44). The `+0x1B0` group receives a copy of the 24 position bytes at state `+0x000–+0x017`, so a joining player's creature no longer carries unspecified bytes there. In the thirteen-item block, the items at `+0x980` and `+0xA98` each receive selector bytes `+0x00` = `0x03` and `+0x01` = `0x02` (one two-byte store of `0x0203`) and byte `+0x0D` = `0x01`. State `+0x130` is set to `0x01`. [Evidence: server RVA `0x025A23–0x025A8A`.]
6. The routine at server RVA `0x00A840` is then called with state `+0x054` and the appearance block. Its outgoing calls include the imported random function, so the join path, unlike construction, consumes random draws; its draw count and resulting appearance are not closed here. Later join work inserts records into creature storage beyond the codec state. [Evidence: server RVA `0x025A91`; `decompiled/server/calls.csv`, outgoing calls of server RVA `0x00A840`.]

Other construction sites exist in both images (client sites in the functions at RVAs `0x059C40`, `0x06FC50`, `0x0821A0`, `0x087520`, `0x20C510` and one unattributed site at `0x21592D`; server sites in the function at server RVA `0x1322D0` and one unattributed site at server RVA `0x13B6ED`), and the client state initializer is also called directly on other storage by the functions at RVAs `0x04BE40`, `0x068B70`, `0x069C10` and `0x06FC50`. The construction defaults above apply at the moment each constructor or initializer returns; any later assignments made by those callers are not covered. The initialized values and omitted assignments above are the bounded defaults established by the supported client and server images.

## Re-certification (2026-11-21)

Every claim of the previous text was re-checked against `binary/Cube.exe` (instruction bytes and the original assembly listing) and the original client exports; the dedicated server was checked against `binary/Server.exe`. The SHA-256 values of both images were recomputed and match the values above. Infinite-Cubes reconstruction material supplied only a naming lead for state `+0x050`; no fact here depends on it, and the name "controller byte" is used only as an identifier.

Corrections:

1. Status marker placed directly under the title in the required form. [Document form.]
2. The constructor does not store the creature ID first: it first stores a fixed type-table pointer at creature `+0x00` (client VA `0x006FD8CC`), then copies the ID as two four-byte halves to creature `+0x08`. [Evidence: client RVA `0x03B6BD–0x03B6CE`.]
3. The cited evidence range for the 24-byte `+0x1B0` copies began at the `+0x1A8` copy; the six-word copy is at `0x0662D9–0x06631B`. [Evidence: client RVA `0x0662CD` copies `+0x1A8`; `0x0662D9` begins `+0x1B0`.]
4. The dispatch evidence range ended mid-instruction; the indirect jump spans `0x06B8B7–0x06B8BD`, and the dispatcher bound (`0x0F`) and kind-0 table mapping are now stated. [Evidence: client RVA `0x06B8A7–0x06B8BD`, table byte `0x06D1A4`.]
5. Added that the eight-byte ID local is cleared before the ID read. [Evidence: client RVA `0x06BBB5–0x06BBD3`.]
6. The allocation-failure statement is made precise: a null pointer is inserted under the ID before the state address is formed from it. [Evidence: client RVA `0x06BC19–0x06BC52`.]
7. Added the dedicated-server construction section; the server constructor, state, appearance and thirteen-item initializers are store-for-store identical to the client's, with the corresponding helper order and zero random draws. [Evidence: server RVAs `0x006400`, `0x007020`, `0x006970`, `0x006AD0`; `decompiled/server/calls.csv`.]
8. Added the default controller byte subsection: state `+0x050` is a one-byte `0x03` store in both images, zero selects a separate branch in both maximum-health queries, and the server join path assigns zero. [Evidence: client RVA `0x03C191`, `0x044DB0`; server RVAs `0x0070B1`, `0x00FDA0`, `0x025A19`, `0x02561A`.]
9. Added the server player-join overrides, explicitly separated from construction defaults, including the thirteen-item block being zeroed (level `0`), the `+0x1B0` group receiving the position, the health query running before the controller byte is cleared, and the join path's random-consuming appearance routine. [Evidence: server RVAs `0x0110D0`, `0x011360`, `0x0258FA–0x025A91`, `0x00A840`.]
10. Stated that the `+0x1B0` group uses the same three-eight-byte writer and reader as the position at `+0x000`. [Evidence: client RVA `0x0CDAA0` calls `0x0CBC80` for both; `0x0CCFA0` calls `0x0CC5C0` for both.]
11. Added the shared writer/reader group order, its one-bit-per-group mask consumption, and how it differs from the copier order. [Evidence: client RVAs `0x0CDAA0`, `0x0CCFA0`, `0x0CC5C0`.]
12. Added that the first four appearance bytes come from one four-byte store of `0xFFFF0000`, and that `+0x130/+0x131` and `+0x17C/+0x17D` are each written as one two-byte zero. [Evidence: client RVA `0x028750`, `0x03C1E1`, `0x03C276`.]
13. Added that each item's count store precedes its component clear, and cited the item copier's byte loop. [Evidence: client RVA `0x03C31B–0x03C333`, `0x03BC35–0x03BC3F`, `0x00EED0–0x00EEF7`.]
14. Added the other client and server construction and direct-initializer call sites as explicitly uncovered callers. [Evidence: `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`, calls to client RVAs `0x03B690`, `0x03C100` and server RVA `0x006400`.]

Remaining uncertainties:

- The kind-0 branch is not covered by the exported decompile or the listing; it was decoded from raw instruction bytes. Malformed or truncated body handling, including the ID value when the body is shorter than eight bytes, is not closed.
- The gameplay meaning of the controller byte values (`3` at construction, `0` for a joining player) and of all other unnamed properties is not closed.
- Unwritten bytes have no specified value; the allocator's behavior is an external dependency.
- For the server join path: the source of the spawn position values, the rounding of the conversion at server RVA `0x14A946`, the computed maximum-health value, the draw count and effects of server RVA `0x00A840`, and the remainder of the join work are not closed.
- Assignments made by the other construction sites after their constructor returns are not covered.
- No other held specification is cited by this contract, so no cross-specification comparison was performed for it.
