Status: FINAL / STABLE / HELD

# Alpha voxel fallback records: initialization and classification

This is an independent, code-free addendum contract. It establishes the contents, initialization and classification of the six static four-byte voxel fallback records used by (a) the client ray query at RVA 0x1A35D0 and (b) the separate voxel lookup at RVA 0x02F7E0 that the late camera range of the frame routine reaches through the fixed-coordinate adapter at RVA 0x02F860 before it calls the ray query.

## Correction notice for the held ray contract

The held contract `docs/specs/alpha-camera-ray-geometry.md` states that no instruction stores to the ray routine's three static records by absolute address, treats those records as remaining zero, and concludes that missing, below-column and above-column cells classify as non-blocking. The first statement is wrong: it came from a listing search whose pattern did not match the way the listing prints these addresses. The records are written by three initializer routines before the program's main routine runs. Where the two documents differ, this addendum governs. In particular, for the ray routine a missing cell, a cell outside the horizontal range, and a cell below its column are **blocking**, not non-blocking. The held contract's bullet list for the two camera call shapes and its first two "Unresolved" items about these records must be read with that replacement. Everything else in the held contract is unaffected.

## Image and evidence

The supported original is `binary/Cube.exe` under the evidence root `disassembled alpha cube world`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base 0x00400000. Addresses are RVAs unless marked VA. Evidence: the image bytes themselves (initializer routine bodies, the pointer table, the entry routine's table arguments, the import name strings, the file-backed contents of the data section) and the instruction listing `decompiled/client/assembly.txt` for the same ranges and for 0x02F7E0-0x02F856 and 0x1A36A0-0x1A378D. Another project contract was used only to learn that initializer routines exist near 0x2FAAD0; every fact below was re-derived from the image. No reconstruction material, runtime observation or implementation material was used.

## The six records

Each record is four bytes. Only byte 3 takes part in classification: its low five bits give a type value, and a cell is non-blocking when that value is 0 or 2 and blocking for every other value. Both the ray routine (0x1A3774-0x1A378B) and the predicate the camera caller applies to its lookup result (0x03B480, read in the listing) use this same rule.

| Record VA | Used by | Selected for | Initializer RVA | Bytes 0,1,2,3 after initialization | Type value | Classification |
| --- | --- | --- | --- | --- | --- | --- |
| 0x0076B7D4 | ray routine | horizontal index out of range, null region or chunk, null column address, or height below the column base | 0x2FAB00 | C8 C8 C8 01 | 1 | blocking |
| 0x0076B7CC | ray routine | height at or above the column top with height zero or negative; also the substitute for an in-column record with type value 0, height zero or negative, and bit 0x40 of byte 3 clear | 0x2FAB30 | FF FF FF 82 | 2 | non-blocking |
| 0x0076B7D0 | ray routine | height at or above the column top with positive height | 0x2FAAD0 | FF FF FF 00 | 0 | non-blocking |
| 0x0076AFC4 | caller's lookup 0x02F7E0 | null result from its column lookup 0x0347A0 (whose own admission rules were not re-read here), or height below the column base | 0x2F9750 | C8 C8 C8 01 | 1 | blocking |
| 0x0076AFBC | caller's lookup 0x02F7E0 | height at or above the column top with height zero or negative; also the same substitution as above | 0x2F9780 | FF FF FF 82 | 2 | non-blocking |
| 0x0076AFC0 | caller's lookup 0x02F7E0 | height at or above the column top with positive height | 0x2F9720 | FF FF FF 00 | 0 | non-blocking |

The two families are distinct storage with identical contents and identical case mapping, so for these fallback cases the ray routine and the caller's lookup classify alike. "Column top" means the 32-bit sum of the column's base height and its record count. The case selection for the ray routine is at 0x1A36A0-0x1A3774; for the caller's lookup it is at 0x02F7E0-0x02F856, and the adapter 0x02F860 does call 0x02F7E0 directly.

In the non-blocking record with byte 3 equal to 0x82, bit 0x40 is clear and bit 0x80 is set. What bytes 0 to 2 and the upper three bits of byte 3 mean was not examined.

## What each initializer does

Each of the six initializers takes no arguments, writes exactly one record through absolute addresses, and does nothing else.

- 0x2FAAD0 and 0x2F9720: store the 16-bit value 0xFFFF to bytes 0-1, the byte 0xFF to byte 2, and the byte 0x00 to byte 3 of their record.
- 0x2FAB30 and 0x2F9780: store 0xFFFF to bytes 0-1, 0xFF to byte 2, and 0x82 to byte 3.
- 0x2FAB00 and 0x2F9750: store the 16-bit value 0xC8C8 to bytes 0-1 and the 16-bit value 0x01C8 to bytes 2-3, giving byte 2 equal to 0xC8 and byte 3 equal to 0x01.

All six bodies were read from the image bytes and agree with the listing.

## When the initializers run

The image contains a table of routine addresses in `.rdata` occupying VA 0x006FC630 through 0x006FC8C0 inclusive, preceded by a zero entry at VA 0x006FC62C and followed by a zero entry at VA 0x006FC8C4. The six initializers appear in it at these positions:

| Table entry VA | Routine RVA | Record written |
| --- | --- | --- |
| 0x006FC68C | 0x2F9720 | 0x0076AFC0 |
| 0x006FC690 | 0x2F9780 | 0x0076AFBC |
| 0x006FC694 | 0x2F9750 | 0x0076AFC4 |
| 0x006FC838 | 0x2FAAD0 | 0x0076B7D0 |
| 0x006FC83C | 0x2FAB30 | 0x0076B7CC |
| 0x006FC840 | 0x2FAB00 | 0x0076B7D4 |

The program entry routine (image entry RVA 0x28E1E0) passes the bounds VA 0x006FC62C and VA 0x006FC8C4 to the runtime import named `_initterm` (through the jump stub at 0x28E8AC, import slot VA 0x006FC3B4) at 0x28E286-0x28E290. This happens after a first call to the import named `_initterm_e` on a separate table at VA 0x006FC8C8-0x006FC8DC, is guarded by the runtime's startup-state word at VA 0x0076E2E8 so that it runs once on the normal startup path, and precedes the entry routine's call at 0x28E307 to the program's main routine at RVA 0x0C8AE0. That main routine is the outer routine that owns window-message processing and frame dispatch. All six records therefore hold their initialized bytes before any gameplay frame, and in particular before any execution of the frame routine 0x088EE0.

The walking of the table itself is performed by the imported runtime routine, not by code in this image; that it calls every nonzero entry between the bounds in ascending order is that routine's documented behavior and is a dependency here. Which module supplies the import was not checked beyond its name.

Before the initializers run the six records are zero: the ray routine's three lie beyond the file-backed part of `.data` (zero-filled at load), and the caller lookup's three lie inside the file-backed part, where the image holds zero bytes. Whether any other routine in the same table, running earlier than these entries, could consult the records while they are still zero was not examined.

## Search for other writers

Method. The whole image file was scanned for every little-endian four-byte occurrence of each of the 24 addresses VA 0x0076B7CC-0x0076B7D7 and VA 0x0076AFBC-0x0076AFC7. This catches any absolute-address operand or stored pointer anywhere in the file, including bytes the listing does not decode. It found 77 occurrences, all in `.text`, and the listing contains the same 77.

Result. Sixteen occurrences are the store operands inside the six initializers. The other sixty-one are each immediately preceded by the opcode byte for loading a 32-bit constant into a register (0xB8 or 0xB9); in the listing they are all loads of a record address into a register, located in the ray routine, in the caller's lookup 0x02F7E0, and in routines 0x19A0E0, 0x19AEB0, 0x19C890, 0x19CBF0, 0x19F0A0 and 0x19F850. No occurrence lies in any data section, so no pointer table refers to these records. No store to any of the 24 addresses exists outside the initializers in absolute-address form.

Limits. The scan cannot see a store made through a register that holds a record address. The ray routine itself only reads through such a register. The caller's lookup returns the record address to its caller, and the six other routines listed were not audited, so a write through a returned or loaded address is not excluded by this method. The scan also cannot see an address formed arithmetically from a different constant (for example a neighbouring base plus an index), nor writes from other modules. No evidence of any such write was found; none was ruled out.

Two neighbouring static records at VA 0x0076AFC8 and VA 0x0076AFD0, with a guard word at VA 0x0076AFCC, are separate storage that helper 0x02F730 writes lazily on its out-of-range branches and that routine 0x09D910 also references. They are not among the six records above and are not reached by the ray routine's or the caller lookup's in-range use of that helper.

## Consequences for the two camera call shapes

Provided no unlisted writer exists, the ray routine's walk with first option 1 (advance while blocking) treats the fallback cases as follows:

- A cell with missing data, outside the horizontal index range, or below its column base is blocking, so the walk continues through it. A ray that leaves loaded data sideways or downward therefore does not stop at the edge of the data; it keeps advancing until it meets a non-blocking cell, the maximum, or the step cap.
- A cell at or above its column top is non-blocking at any height, so the walk stops there.
- An in-column record with type value 0 is non-blocking whether or not the substitution applies, because the substitute has type value 2.

The caller's own pre-test classifies the same cases the same way, so a corner sample or eye position lying in missing data or below a column is treated as blocked by the caller and then advanced through by the ray routine.

## Unresolved

- Writes to any of the six records through a register-held, returned, or arithmetically formed address; the six other address-loading routines were not audited.
- Whether any earlier entry of the initializer table reads these records before they are set.
- The exact contract of the imported table-walking routine and the module that supplies it.
- The meaning of record bytes 0 to 2 and of bits 0x20, 0x40 and 0x80 of byte 3.
