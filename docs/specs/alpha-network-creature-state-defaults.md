# Alpha network creature-state defaults

Status: FINAL / STABLE / HELD. This original-only contract establishes the state supplied by a newly constructed creature to the client packet-kind-0 receive path. It describes initialized properties and explicitly unspecified storage. It does not define a zero-filled creature image or a complete receive protocol.

The supported original image is `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, under the evidence root `disassembled alpha cube world`. Addresses below are RVAs in that image, whose preferred image base is `0x00400000`. Evidence paths are relative to that root. All findings were independently checked in original client exports and, for the construction and copy boundaries, the original executable. No prior contract or replacement implementation supplies these facts.

## Receive boundary and address conventions

Packet kind 0 dispatches to RVA `0x06BAAD`. After receiving and processing its body buffer, the path reads an eight-byte creature ID, locks the creature collection, and looks up that ID. When the lookup returns no creature, it requests `0x1E60` bytes, calls the creature constructor at RVA `0x03B690` with the received ID, and inserts the resulting pointer under that ID. Existing creatures bypass this construction. The allocation call itself does not establish any initial value for its storage. [Evidence: `binary/Cube.exe`, dispatch at `0x06B8A7–0x06B8B7`, tables at `0x06D180` and `0x06D1A4`, kind-0 branch `0x06BAAD–0x06BC4B`; `decompiled/client/functions/0046b740.c`, RVA `0x06B740`, whose exported body does not cover the whole native switch.]

At RVA `0x06BC52`, both newly constructed and existing creatures supply the address at creature `+0x10`. The property copier at RVA `0x066050` copies that state into a stack-resident temporary. The receive path unlocks the collection at `0x06BC6F`, then calls the state reader at RVA `0x0CCFA0` on that temporary. Thus the defaults below are the temporary's starting properties when a creature was newly constructed. They are not reapplied to an existing creature on every packet. They also do not describe the final decoded state when the packet supplies replacements. [Evidence: `binary/Cube.exe`, RVA `0x06BC52–0x06BC87`; `decompiled/client/functions/00466050.c`, RVA `0x066050`; `decompiled/client/functions/004ccfa0.c`, RVA `0x0CCFA0`.]

Throughout this contract, a **state offset** is relative to creature `+0x10`. For example, state `+0x15C` is creature `+0x16C`. The copied state extends through state `+0x1167`, giving an extent of `0x1168` bytes. The creature's own eight-byte ID is at creature `+0x08`, before this state. The constructor copies the received ID there; it does not substitute that ID for either of the separate eight-byte properties within the codec state. [Evidence: `binary/Cube.exe`, RVA `0x03B6BD–0x03B6D1`; `decompiled/client/functions/00466050.c`, RVA `0x066050`, last copies at `0x0663DE–0x0663F6`.]

## Initialized outer state

The creature constructor invokes the state initializer at RVA `0x03C100` on creature `+0x10`. Later successful constructor work concerns storage beginning at creature `+0x1178` and above; the inspected game-owned callees do not replace the initialized codec state. All binary32 zero values below have bits `0x00000000`, and all binary32 one values have bits `0x3F800000`. Values are direct fixed-width stores, without a maximum-health query, stat calculation, numeric conversion, or random selection. [Evidence: `binary/Cube.exe`, RVA `0x03B690–0x03BBF6` and `0x03C100–0x03C379`; `decompiled/client/functions/0043b690.c`; `decompiled/client/functions/0043c100.c`.]

The table covers the outer properties selected by the codec writer at RVA `0x0CDAA0`. Where this bounded evidence does not close a property's gameplay name, its address and width identify it without assigning an invented meaning. Rows containing multiple offsets describe separate properties at each listed offset, rather than a wider zero-filled allocation.

| State offset | Property width or grouping | Newly constructed value |
| --- | --- | --- |
| `+0x000` | Three eight-byte position components | All three zero |
| `+0x018`, `+0x024`, `+0x030`, `+0x03C` | Four separate triplets of binary32 components | Every component positive zero |
| `+0x048` | Binary32 scalar | Positive zero |
| `+0x04C` | Four-byte property | Zero |
| `+0x050` | One-byte numeric role | `3` |
| `+0x054` | Four-byte creature-type property | Zero |
| `+0x058` | One-byte action value | Zero |
| `+0x05C`, `+0x060`, `+0x064` | Three four-byte properties | Zero each |
| `+0x068` | `0xAC`-byte appearance block | Fixed appearance properties listed below; one byte unspecified |
| `+0x114` | Two-byte property | Zero |
| `+0x118` | Four-byte property | Zero |
| `+0x11C` | Signed four-byte property | `-3000`, bits `0xFFFFF448` |
| `+0x120`, `+0x124`, `+0x128` | Three four-byte properties | Zero each |
| `+0x12C` | Binary32 property | Positive zero |
| `+0x130`, `+0x131` | Two separate one-byte class/specialization values | Zero each |
| `+0x134` | Binary32 property | Positive zero |
| `+0x138`, `+0x144`, `+0x150` | Three separate triplets of binary32 components | Every component positive zero |
| `+0x15C` | Binary32 health at creature `+0x16C` | `500`, bits `0x43FA0000` |
| `+0x160`, `+0x164` | Two binary32 resource properties | Positive zero each |
| `+0x168` | Group of five binary32 properties | In address order: `100`, `1`, `1`, `1`, `1`; first bits `0x42C80000` |
| `+0x17C`, `+0x17D` | Two separate one-byte properties | Zero each |
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

The decimal approximations do not replace the listed binary32 bit patterns. The initializer stores those patterns directly and makes no random draws or arithmetic calculations for them. The appearance property copier transfers every listed initialized property but skips byte `+0x05`. The codec appearance group can nevertheless transmit or receive all `0xAC` bytes as a raw block. Its full raw payload therefore includes an unspecified byte when originating from this default construction/copy chain. [Evidence: appearance initializer and copier above; `decompiled/client/functions/004cb930.c`, RVA `0x0CB930`; `decompiled/client/functions/004cc1c0.c`, RVA `0x0CC1C0`.]

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

The zero component count is distinct from the initialized component storage: the constructor clears all 256 component bytes even though the count is zero. The item copier copies all that storage and the count, together with the other listed item properties. It leaves the five listed bytes untouched in its destination. No item seed is randomly generated, no component is selected, and no category is inferred from allocation. The whole-item and thirteen-item codec groups use raw payload extents of `0x118` and `0xE38` bytes, respectively; these raw extents include the unspecified item bytes. Their equality helpers do not establish a zero value for those omitted-copy bytes. [Evidence: `decompiled/client/functions/0040ee70.c`, RVA `0x00EE70`; `0042f4a0.c`, RVA `0x02F4A0`; `004cba70.c`, RVA `0x0CBA70`; `004cb9d0.c`, RVA `0x0CB9D0`; `004ccaa0.c`, RVA `0x0CCAA0`; item reader `004cc3c0.c`, RVA `0x0CC3C0`.]

## Unspecified state storage and conditional decode

Apart from the per-item gaps just listed, the successful constructor chain leaves these state bytes unwritten: `+0x051–+0x053`, `+0x059–+0x05B`, appearance byte `+0x06D`, `+0x116–+0x117`, `+0x132–+0x133`, `+0x17E–+0x17F`, `+0x199–+0x19B`, `+0x1AC–+0x1C7`, and `+0x1C9–+0x1CB`. No zero-fill of the `0x1E60`-byte creature allocation occurs in the inspected path. [Evidence: `binary/Cube.exe`, RVA `0x06BC02–0x06BC26`, `0x03B690–0x03BBF6`, `0x03C100–0x03C379`, and `0x028750–0x0288A9`; `decompiled/client/functions/0043bc00.c`.]

Most of these gaps are also skipped by the state property copier. State `+0x1B0–+0x1C7` is the exception: all 24 bytes are copied as six four-byte words despite having no constructor assignment. The four bytes at `+0x1AC–+0x1AF` are not copied and are not a writer-selected property. The codec explicitly recognizes the 24-byte group beginning at `+0x1B0`. Its initial contents therefore remain unspecified data at a real codec boundary, rather than being dismissed as padding. [Evidence: `binary/Cube.exe`, RVA `0x0662CD–0x06631B`; `decompiled/client/functions/004cdaa0.c`, RVA `0x0CDAA0`, group at state `+0x1B0`; writer `004cbc80.c`, RVA `0x0CBC80`; reader `004cc5c0.c`, RVA `0x0CC5C0`.]

The temporary state is populated by property copies, not by copying the entire `0x1168`-byte extent or by clearing that extent first. Consequently, an omitted-copy padding byte in the temporary need not even equal the corresponding unspecified byte in the newly allocated creature. No reproducible value, cross-allocation equality, previous-stack value, or allocator pattern is specified for any unwritten byte. [Evidence: `binary/Cube.exe`, receive-worker prologue `0x06B740–0x06B8B7` and copy call `0x06BC52–0x06BC62`; complete state copier `0x066050–0x066403`; nested property copiers cited above.]

The state reader uses the packet's 64-bit presence mask to decide which properties to replace. An absent group advances the field position without assigning that group, retaining the seeded value, including an unspecified value where the seed was unspecified. Present raw appearance or item groups can replace their full extents, including padding. This is why initialized semantic defaults, copy behavior, and raw serialized bytes are separate facts. This contract does not close malformed or truncated state-body handling, final application after decode, or remote interpolation. [Evidence: `decompiled/client/functions/004ccfa0.c`, RVA `0x0CCFA0`; representative conditional readers `004cc5c0.c`, `004cc1c0.c`, and `004cc3c0.c`, RVAs `0x0CC5C0`, `0x0CC1C0`, and `0x0CC3C0`.]

## Construction order, random draws, and dependency limits

The successful relevant call order is fixed. The creature constructor first stores the received creature ID, then invokes the state initializer. That initializer establishes the leading scalar/vector properties and calls the appearance initializer once. It then establishes the middle scalar properties and the standalone item's properties, clears that item's component storage, and calls the thirteen-item initializer. The thirteen-item initializer visits ascending item storage order and clears each item's component storage before proceeding to the next. On returning, the state initializer clears the final four-byte property at `+0x1154`, clears both eight-byte portions of the name storage, and clears the eleven-word block at `+0x1128`. [Evidence: `binary/Cube.exe`, RVA `0x03B6BD–0x03B6D1` and `0x03C100–0x03C379`; `decompiled/client/functions/0043bc00.c`, RVA `0x03BC00`.]

The outer constructor then creates its empty resident/container state and initializes its other runtime members. Its successful game-owned helper order after state initialization is RVA `0x039600`; `0x03C870` twice; `0x03C020`; `0x03C8A0`; `0x03C870`; `0x25AE80` twice; `0x2391E0`; `0x06D4B0`; `0x03C8E0`; `0x278520`; `0x03C8E0`; and `0x011E50`, followed by the final component-storage clear outside the codec state. These helpers establish empty sentinels or fixed runtime-member values. Their allocated addresses and non-state storage are not codec defaults. [Evidence: `decompiled/client/functions/0043b690.c`, RVA `0x03B690`; original helper exports `00439600.c`, `0043c870.c`, `0043c020.c`, `0043c8a0.c`, `0065ae80.c`, `006391e0.c`, `0046d4b0.c`, `0043c8e0.c`, `00678520.c`, and `00411e50.c`; `binary/Cube.exe`, RVA `0x03B6D6–0x03BBDC`.]

There are **zero game random draws** in this traced successful construction and state-copy chain. The state, appearance, item, and property-copy routines make fixed stores or copies. The additional constructor helpers inspected above allocate/link empty sentinels or write fixed members; none calls the game random generator or the imported random function. The copy order is outer state properties, appearance properties, later outer properties, standalone item, thirteen items in ascending storage order, the eleven-word block, the final four-byte property, and the name storage. This no-draw finding is limited to the traced chain; it does not define unrelated threads' random consumption or allocator internals. [Evidence: original routines cited in this section; state copier `decompiled/client/functions/00466050.c`, RVA `0x066050`; nested copiers `00459800.c`, `0040ee70.c`, and `00459b10.c`; `decompiled/client/calls.csv`, outgoing calls from the named constructor and initialization routines.]

Allocation success, imported allocation/clear/copy implementations, exception behavior, and the lifetime or concurrent mutation of the surrounding creature collection remain external dependencies. The allocation-failure path is not a source of valid default state: the native branch can leave a null pointer before the later state-address use, and this contract supplies no invented recovery behavior. No imported power or other stat arithmetic is called to establish these values. Meanings of unnamed properties, anatomy identifiers, equipment-slot names, and all post-decode gameplay behavior remain explicitly unclosed. The initialized values and omitted assignments above are the bounded defaults established by the supported client image.
