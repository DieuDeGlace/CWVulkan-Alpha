# Alpha backing-reference producers and preservation boundaries

## Evidence and scope

This contract concerns the original 32-bit client image `binary/Cube.exe` under the relative evidence identifier `disassembled alpha cube world`. Its SHA-256 is `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`; its size is 3,885,568 bytes and preferred image base is `0x00400000`. Addresses below are RVAs. Original instructions in that image and `decompiled/client/assembly.txt` were checked against the individually identified original function files cited below. No reconstruction, runtime observations or other project contracts supply facts here.

The closed producer is the creature's three-word reference written during one descriptor-activation route. Its third word really is the current descriptor pointer-array ordinal. In contrast, the audited spatial-record constructors and generation sites initialize spatial member `+0x178` to `0xFFFFFFFF`, and the audited copying paths preserve it. This audit does **not** establish its first nondefault or positive producer. An equality comparison with the creature's third word does not supply that missing producer.

The scope covers the specified fields, their demonstrated object bases, direct initialization, an activation assignment, selected generation and relocation paths, and data reconstruction helpers. It does not claim a complete inventory of every indirect writer, world generator, storage format or caller.

## Object bases and widths

The following are different objects and offset origins. The letters in this document are explanatory identifiers, not recovered source type names.

| Object | Relevant storage | Established interpretation |
|---|---|---|
| Creature C | `C+0x1B0`, `C+0x1B4`, `C+0x1B8` | Three adjacent 32-bit words. Activation supplies two cell coordinates and a descriptor-array ordinal. |
| Creature data D | D starts at `C+0x10` | The same three words are `D+0x1A0`, `D+0x1A4`, `D+0x1A8`. |
| Cell Q used by activation | Coordinates at `Q+0x60` and `Q+0x64`; pointer vector beginning at `Q+0x18` | The coordinate loads and each vector element are 32 bits. The vector contains descriptor pointers. |
| Spatial value record M | Extent `0x188`; relevant word at `M+0x178` | A separate 32-bit member, copied independently of the creature triple. |
| Cell's spatial value vector | Begin/end/capacity at cell `+0x0C/+0x10/+0x14` | Elements advance by `0x188`; this is distinct from the descriptor pointer vector at `+0x18`. |

Creature construction explicitly passes `C+0x10` to the data constructor at RVA `0x03C100` (`0x03B6CB–0x03B6D1`). Consequently, a data-relative `+0x1B8` is **not** the requested creature-relative `+0x1B8`. Numeric offset matches in unrelated objects likewise establish no creature or spatial-record assignment. Evidence: `decompiled/client/functions/0043b690.c`, `0043c100.c`, and original instructions at the cited addresses.

The activation values undergo no floating-point conversion or narrowing. The descriptor ordinal is iterated and later range-checked as a signed 32-bit integer. Copying treats all three words as raw 32-bit quantities. Spatial `M+0x178` is also loaded/stored as a full word; its comparison with `C+0x1B8` tests full-width equality, without a sign conversion.

## Creature initialization and explicit reset

Fresh creature data gives the reference the values **(-1, -1, 0)**. The constructor first writes zero to `D+0x1A8` at `0x03C2B5`, then writes `0xFFFFFFFF` to `D+0x1A0` and `D+0x1A4` at `0x03C2BB` and `0x03C2C5`. The zero value is established before those stores; this is not an uninitialized third word. Evidence: `0043c100.c` and original `0x03C1BA–0x03C2C5`.

The broader creature reset at RVA `0x046330` passes `C+0x10` to RVA `0x0465C0` before its later creature-state work. That data reset writes `D+0x1A0=-1`, then `D+0x1A4=-1`, then `D+0x1A8=0`, at `0x046701`, `0x04670B` and `0x046715`. Thus this reset removes an earlier positive ordinal and restores the same triple as fresh construction. It does not preserve the old reference pending a later activation. Evidence: `00446330.c`, `004465c0.c`, original `0x046338–0x04633D` and `0x046701–0x046715`.

There is a demonstrated reconstruction entry at RVA `0x04BE40`: its target creature is reset through `0x046330` at `0x04BE7F`; later a fresh temporary data object is constructed by `0x03C100`, then assigned to the target's data subobject through `0x04B040` at `0x04BF84–0x04BF93`. Those operations independently restore the default triple before the later field-reading work. This observation does not specify the complete format or every effect of that reader. Evidence: `0044be40.c` and the corresponding original instructions.

Zero in the third word is therefore both its constructor/reset value and a valid first-array-element ordinal. It cannot by itself distinguish an activated reference from default state. The two coordinate defaults are also values; this audit does not establish that coordinate -1 is globally impossible for a real cell.

## Descriptor activation supplies a real array ordinal

Inside world update RVA `0x20C510`, the descriptor traversal resets a 32-bit ordinal to zero at `0x20FB94`. It obtains the descriptor-pointer count from the vector at `Q+0x18`; the count helper `0x087F50` derives the count from end minus begin, divided by four. The index helper `0x068C60` selects a four-byte pointer slot using that same ordinal. The selected pointer is read at `0x20FBB9`. The ordinal advances at `0x210439`, and the next iteration is admitted only while it is signed-less-than the current count (`0x210444–0x210453`).

On the audited successful creature-creation route, `0x20FFBA` requests `0x1E60` bytes. The creature constructor receives the selected descriptor's identifier beginning at descriptor `+0x48`; that identifier occupies two 32-bit words. The new creature is then associated with that identifier, and descriptor properties are transferred before the reference assignment. These identity and property transfers do not provide the third reference word. Evidence: original `0x20FFBA–0x2100B5`.

At `0x2100BA–0x2100FA`, the three values supplied to the reference constructor/copy helpers are, in destination order:

| Creature destination | Actual source |
|---|---|
| `C+0x1B0` | Full 32-bit first coordinate at `Q+0x60`. |
| `C+0x1B4` | Full 32-bit second coordinate at `Q+0x64`. |
| `C+0x1B8` | The current traversal ordinal of the selected descriptor pointer in `Q+0x18`. |

The coordinate access helper `0x028970` uses four-byte component spacing. Helper `0x02C500` constructs the three-word temporary in the supplied order. Helper `0x011E30` then copies the first, second and third word in that order into `C+0x1B0`. Each helper preserves the supplied bits. Evidence: `00428970.c`, `0042c500.c`, `00411e30.c`, `00487f50.c`, `00468c60.c`, and the original activation instructions cited above.

This closes a genuine nondefault source for `C+0x1B8`: successful activation at ordinary ordinal 1 or greater writes that positive ordinal. Activation at ordinal zero writes zero. The traversal increments across descriptors whose activation is skipped, so the stored value is the descriptor's array position, not a count of creatures successfully created. It is not an offset divided by the spatial-record extent, an ordinal into the `0x188`-byte value vector, a descriptor identifier, or a copied descriptor scalar. No remapping to a retained prefix or removed tail is present in this assignment.

The two coordinates come from Q, not from the creature's current position and not from the descriptor's position. The audited assignment does not read or write any `M+0x178`. Its small construction/copy helpers contain no random call. Earlier activation admission, descriptor-property generation and later world processing are outside that narrow no-random-call statement. Allocation failure and malformed vector state are not made safe by this contract.

## Preservation through creature data copies and field reconstruction

Data assignment RVA `0x04B040` copies `D+0x1A0`, `D+0x1A4` and `D+0x1A8` as three consecutive full-width loads and stores at `0x04B2A6–0x04B2C4`. Data copy construction RVA `0x066050` does the same at `0x0662B5–0x0662D3`. Neither calculates coordinates, selects a descriptor, validates the ordinal or substitutes a default for zero or negative bits. Evidence: `0044b040.c`, `00466050.c`, and original instructions.

One concrete data-copy caller is `0x066E17–0x066E2D`: the source is the creature pointer at client `+0x8006D0`, advanced by `0x10`; the destination is an indexed stored object, also advanced by `0x10`. Another is the sender at `0x069EF5–0x069F19`, which copies the same client-designated creature's data into a temporary through `0x066050`. This temporary is supplied to the field serializer at `0x069F81`. These callers confirm that data-relative offsets in these helpers really include the requested creature triple. They do not establish a new ordinal origin. Evidence: `00466d90.c`, `00469c10.c`, and original instructions.

The field serializer RVA `0x0CDAA0` treats the three words at data `+0x1A0` as one 12-byte group, using helper `0x0CBB10` at `0x0CDFAA–0x0CDFC5`. With its force byte clear, that helper compares all three 32-bit words with the comparison data. Equal triples omit the group; any differing word includes the entire group. A nonzero force byte includes it without requiring a difference. Inclusion supplies the new triple's bytes and sets the corresponding presence bit. No ordinal is synthesized from the serializer's field counter or output-list position. Evidence: `004cdaa0.c`, `004cbb10.c` and original `0x0CBB16–0x0CBB67`.

The memory-buffer field reader RVA `0x0CCFA0` addresses data `+0x1A0` at `0x0CD301–0x0CD311` and uses the 12-byte helper `0x0CC540`. If the group's presence bit is clear, its destination is unchanged. If the bit is set and all 12 bytes are available, the first eight bytes and final four bytes replace the triple without interpretation. If fewer than 12 bytes are available, this helper moves the input cursor to the available end and leaves the whole destination group unchanged. These are local reader semantics, not a guarantee about the validity or later publication of a truncated enclosing message. Evidence: `004ccfa0.c`, `004cc540.c`, original `0x0CC56E–0x0CC5B8`.

The socket field reader RVA `0x0CD3E0` similarly supplies data `+0x1A0` to helper `0x0CC4C0` at `0x0CD959–0x0CD969`. With the presence bit set, that helper requests 12 bytes and advances the destination as reads arrive; it does not stage the group for atomic replacement. A failed or closed read can therefore leave a partial group locally written. With the bit absent it makes no destination write. This audit does not close the receiver's enclosing error handling or publication decision. Evidence: `004cd3e0.c`, `004cc4c0.c` and original helper instructions.

These reconstruction helpers can preserve or replace arbitrary full-width third-word bits; they do not demonstrate where an external nondefault value was originally created. Their absence-of-validation is local to these helpers. No conclusion about every save, packet type or remote caller follows from it.

## Spatial record initialization, generation and copying

The `0x188`-byte record constructor RVA `0x18EAA0` explicitly sets `M+0x178`, `M+0x17C` and `M+0x180` to `0xFFFFFFFF`, in that order, then sets `M+0x184` to zero (`0x18EACB–0x18EAE9`). This is not a claim that all record bytes are zeroed or initialized by that constructor. In particular, the meaning of other tail members is not substituted for `+0x178`. Evidence: `0058eaa0.c` and original instructions.

Additional record-building routines at `0x1BF5B0`, `0x1BFD70` and `0x1BFF10` explicitly use the same `-1,-1,-1,0` tail initialization. Their later choice of record kind and dimensions does not replace `+0x178`. Random choices inside these routines do not supply that member. Evidence: `005bf5b0.c`, `005bfd70.c`, `005bff10.c`, and original stores at `0x1BF5EB`, `0x1BFDAB`, `0x1BFF4A` with their following tail stores.

The record copy constructor RVA `0x18EA10` reads the source's `+0x178` at `0x18EA58` and stores the same full word at destination `+0x178` at `0x18EA61`. It then separately copies the following tail words. This copy does not recalculate `+0x178` from the destination address, record kind, position, vector size or copy count. Its handling of nested members does not imply that every byte of record padding is copied. Evidence: `0058ea10.c` and original `0x18EA58–0x18EA85`.

The following scoped generation sites also have an identified source for the actual word appended to the spatial value vector:

| Original route | Established origin of appended `+0x178` |
|---|---|
| RVA `0x1AD320`, direct temporary append | A temporary's word explicitly initialized to `0xFFFFFFFF` at `0x1AD60D`, read at `0x1AD9F0`, and stored at `0x1AD9F3`. |
| RVA `0x1DC6E0`, first scoped temporary append | Initialization to `0xFFFFFFFF` at `0x1DDE83`; the same temporary word supplies the store at `0x1DE242`. |
| RVA `0x1DC6E0`, second scoped temporary append | Initialization to `0xFFFFFFFF` at `0x1DE3CB`; the same temporary word supplies the store at `0x1DE7E4`. |
| RVA `0x1F0CE0`, scoped temporary append | Initialization to `0xFFFFFFFF` at `0x1F1803`; the same temporary word supplies the store at `0x1F1B14`. |

The placement helper RVA `0x1F4DA0`, reached by the first three listed temporary routes, adjusts the supplied record's vertical position words at `+0x18/+0x1C` and examines terrain. It does not assign `+0x178`. The placement/shape helper RVA `0x1CB3B0`, reached by the fourth route at `0x1F1856`, supplies position, dimensions, orientation and active-byte changes; it also does not assign that tail word. The stated default origins concern these ordinary distinct-cell/temporary-record calls; arbitrary overlapping arguments and corrupt pointers are excluded. Evidence: `005ad320.c`, selected regions of `005dc6e0.c` and `005f0ce0.c`, `005f4da0.c`, `005cb3b0.c`, and the corresponding original instruction ranges.

These generation routines also contain append copies from an already existing vector element: the `+0x178` transfers at `0x1AD8BE–0x1AD8C4`, `0x1DE104–0x1DE10A`, `0x1DE69E–0x1DE6A4` and `0x1F19B3–0x1F19B9` preserve that selected source record's word. Such copies can propagate a nondefault value if one already exists, but do not reveal its first producer.

The general append helper RVA `0x1A0E00` appends through copy constructor `0x18EA10` and advances the vector end by `0x188`. For a source that is already an element of the same vector, it preserves the source's element ordinal across any required reallocation so it can find that element again. That internal ordinal is used to relocate the **source address**; it is not stored in `M+0x178`. Reallocation RVA `0x1949D0` uses range-copy helper `0x18E8E0`, which copy-constructs records in `0x188`-byte steps, before old storage is destroyed and the vector pointers are replaced. This preserves each record's tail word across storage relocation. Evidence: `005a0e00.c`, `005949d0.c`, `00594b40.c`, `0058e8e0.c`, and original call sites `0x1A0E95` and `0x18E932`.

The completed per-record builders at `0x1BF7C0` and `0x1F6E20` likewise initialize a temporary tail to `-1,-1,-1,0`, then deliver the record using copy constructor `0x18EA10` at `0x1BFCC0` or `0x1F7379`. Their shape/random branches do not supply a different `+0x178`. Evidence: `005bf7c0.c`, `005f6e20.c` and original instructions.

## What the consumers establish, and what they do not

The fatal-processing routine RVA `0x195A60` uses the creature's two reference coordinates for a cell lookup at `0x196193–0x19619F`. In that cell it traverses the separate spatial value vector. For a kind-5 record it compares `M+0x178` with `C+0x1B8` (`0x1961B4–0x1961C5`) and then tests the record's active byte at `+0x30`. Advancing that scan by `0x188` does not assign its scan ordinal to either field. This establishes an equality relationship used by this consumer, not a producer or a universal uniqueness rule.

A later part of the same routine uses `C+0x1B8` to index a cell's descriptor pointer vector at `+0x18`, after checking signed nonnegativity and strict signed-less-than count (`0x196C13–0x196C2D`). RVA `0x1A4980` demonstrates the same descriptor-vector indexing boundary at `0x1A4A09–0x1A4A22`. Zero passes these index checks when the vector is nonempty; an index equal to the count does not. Negative values are rejected there. The earlier kind-5 equality test itself does not impose those index checks. Evidence: original instructions in `decompiled/client/assembly.txt` at the cited addresses.

Accordingly, the creature field's array-ordinal meaning is independently supported by both its activation producer and these indexed consumers. The spatial member's matching role is supported by equality consumption, but its original nondefault construction remains unclosed. A fresh spatial `-1` does not equal an activated creature ordinal zero. Conversely, full-word equality could compare two `0xFFFFFFFF` values as equal if the enclosing consumer admits them; the comparison alone is not proof of a valid backing relation.

## Explicit limits

No audited route establishes a first positive or other nondefault assignment to spatial `M+0x178`. This is an unresolved dependency, not a finding that the entire image never assigns one. In particular, this contract does not fill that gap by storing a descriptor ordinal, spatial-vector ordinal, generation-loop index or identifier there.

The complete generators spanning RVAs `0x1AF170`, `0x1CC510` and `0x1E4850`, every virtual descriptor producer, every bulk reader and every alias through a pointer into an object have not been closed. The selected generation sites above are not a complete procedural-generation specification. The full storage/network caller graph and short-message publication behavior also remain outside scope. Only the demonstrated local field reconstruction semantics are established.

Copying and vector relocation preserve values; they do not validate a relationship's lifetime. No general guarantee is established that descriptor-array edits rebase surviving creatures, that spatial records are uniquely paired with descriptors, or that a copied reference remains valid after cell removal/reconstruction. Full descriptor activation admission, fatal side effects, deletion ownership and the persistence of nested record objects are not specified here. These gaps must remain separate from the concrete default, ordinal assignment and preservation facts above.

Status: FINAL / STABLE / HELD after complete author readback.
