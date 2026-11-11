Status: FINAL / STABLE / HELD

# World-owned subtype list read by the world-list item producer

This independently authored, code-free contract settles the open point in section 11.5 of `alpha-fatal-loot-branches.md`: which list the world-list item producer reads, how that list gets its contents, what those contents are, and who else reads it. That held contract and its recount note were used only to find the producer and its callers. Nothing in them is relied on as evidence here. The Infinite-Cubes reconstruction was not consulted.

Supported originals: `binary/Cube.exe` (client), SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and `binary/Server.exe` (server), SHA-256 `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1`. Both hashes were recomputed before use. Both images have preferred base `0x00400000`, and every address below is an RVA. Evidence paths are relative to `Documents/disassembled alpha cube world`.

## 1. Evidence method

All instruction-level statements come from `decompiled/client/assembly.txt` and `decompiled/server/assembly.txt`. For every routine range cited as audited, the byte column of the listing was compared mechanically with the bytes of the image at the same address, separately in each image. No mismatch was found. Where the listing omits bytes (alignment padding, and a few three-byte stack adjustments after a storage release), those bytes were read from the image and are accounted for below where they matter.

The list's element values were extracted twice in each image: once from the listing and once by pattern-matching the raw image bytes over the exact append range. The two extractions agree in each image.

Negative results ("no other writer", "no other reader") come from enumerating every instruction in each listing whose memory operand uses displacement `0x88` or `0x8C` from a general register. A raw byte scan of each image's code section for every ModRM byte with a 32-bit displacement of `0x88` or `0x8C` was then matched against that enumeration. Server: 296 byte candidates, of which 233 are listed operands with those displacements. The other 63 fall inside relative-jump displacements or immediate operands, so they are not memory accesses. Client: 592 candidates, 452 listed. Of the other 140, 116 are jump or immediate bytes and 24 fall in bytes the listing does not cover. Each of those 24 lies in a routine that section 5 shows is unrelated to this list, or in code beyond the routines discussed here. Section 8 gives the limits of this method.

## 2. Routines

| Role | Client RVA | Server RVA |
| --- | --- | --- |
| World-list item producer | `0x1F79E0` | `0x12B3F0` |
| Fatal-loot helper (only caller of the producer) | `0x19B180` | `0x0D2AE0` |
| Routine that calls the fatal-loot helper | `0x195A60` | `0x0CEA80` |
| Intermediate caller of that routine | `0x196D30` | `0x0CFD50` |
| World update routine (calls both of the above) | `0x20C510` | `0x1322D0` |
| World constructor | `0x18EB00` | `0x0C8570` |
| World destructor body | `0x193ED0` | `0x0CD940` |
| World deleting destructor (first table slot) | `0x1946C0` | `0x0CE540` |
| World method table (first entry address) | `0x31A794` | `0x1716C4` |
| 32-bit element append | `0x26ADD0` | `0x0F2BE0` |
| Append's capacity growth | `0x23DC90` | `0x026EB0` |
| Second reader of the list (client only) | `0x0E4BD0` | none |
| Random-source import slot (name entry `rand`) | `0x2FC468` | `0x1582F4` |

## 3. Which object the producer receives, and therefore which list it reads

### 3.1 Server

- The producer's only call site is at `0x0D2F2A`, inside the fatal-loot helper. A byte scan for relative calls and jumps to the producer finds only this site, and the image holds no absolute copy of its address. The receiver passed there is the value the helper saved from its own receiver at entry (`0x0D2B0E`). The helper never writes that saved slot again: of its 16 references to the slot, the entry store is the only write.
- The helper's only caller is at `0x0CF24E` in `0x0CEA80`. The receiver passed is `0x0CEA80`'s own receiver, saved at entry (`0x0CEAB0`). Of that slot's 18 references, the entry store is the only write.
- `0x0CEA80` is called from eight sites. Four are in `0x0CFD50`, each passing that routine's own receiver (saved at entry, `0x0CFD8F`). The other four are in or adjacent to the world update routine `0x1322D0` (at `0x132B5B`, `0x13A96C`, `0x13AD32` and `0x147BD4`), each passing the update routine's own receiver. At each of those four sites, the register that carries the receiver is reloaded from the update routine's entry-saved receiver slot just before the call. Of that slot's 196 references, the entry store is the only write.
- `0x0CFD50` is called from seven sites, all in or adjacent to `0x1322D0`, each with the same reloaded receiver.
- The update routine's only call site is at `0x14A0AE`, in `0x149C50`. There the receiver is the object that `0x149C50` itself allocated (`0x80029C` bytes) and handed to the world constructor `0x0C8570` (call at `0x149CBD`, with two zero arguments). The register holding that object is not rewritten between construction and the update call. The constructor's only call site is `0x149CBD`. The only call site of `0x149C50` is `0x14B3DE`.

The producer's receiver is therefore always the single world object built by `0x0C8570`. The list it reads is that object's 32-bit-element sequence. The begin pointer is at object `+0x88`, the end pointer at `+0x8C`, and the capacity-end pointer at `+0x90`.

### 3.2 Client

- The producer's only call site is at `0x19B5CA`, in the fatal-loot helper `0x19B180`. The receiver is the helper's entry-saved receiver (`0x19B1AE`); of its 16 references, the entry store is the only write.
- The helper's only call site is at `0x19622E` in `0x195A60`, which passes its own entry-saved receiver (`0x195A90`; of 18 references, the entry store is the only write).
- `0x195A60` is called from eight sites. Four are in `0x196D30` (at `0x19700F`, `0x197229`, `0x1974DD` and `0x1986D0`), passing that routine's entry-saved receiver (`0x196D6F`). The other four are in or adjacent to the world update routine `0x20C510` (at `0x20CD9B`, `0x214BAC`, `0x214F72` and `0x221E14`). Each of those passes the update routine's entry-saved receiver, reloaded from its slot immediately before the call. Of that slot's 196 references, the entry store is the only write.
- `0x196D30` is called from seven sites, all of which reload that same receiver.
- `0x20C510` is called only from `0x088EE0`, at `0x08CF2E` (inside a counted loop) and at `0x08CF49`. Both sites pass the address `+0x2E4` inside `0x088EE0`'s receiver; that address is computed at `0x089024` and kept in a stack slot.
- The world constructor `0x18EB00` is called only at `0x059F57`, inside the constructor `0x059C40` of the containing object. Its receiver there is the containing object's `+0x2E4`. `0x059C40` is called only from `0x0C8FF9`.

In the client, then, the world is a member at `+0x2E4` of a larger containing object. The producer's receiver is always that member, and the list is the member's sequence at `+0x88` (begin), `+0x8C` (end) and `+0x90` (capacity end).

## 4. Population of the list

### 4.1 Creation and initial state (world construction)

Inside the world constructor, before anything else touches the list, the three pointers at `+0x88`, `+0x8C` and `+0x90` are each set to zero. This leaves an empty sequence with no storage. The list's address is kept in a frame slot that the constructor uses only to reload it for the append run (server `0x0C8628–0x0C864F`; client `0x18EBB8–0x18EBDF`).

### 4.2 Filling (world construction, last step)

The constructor's final work, immediately before its epilogue and single return, is one straight-line run of 45 appends. Each append stores one 32-bit immediate into a stack temporary and passes the temporary's address to the append routine, with the list as receiver. The append routine copies the 32-bit value to the end of the sequence, first growing storage if the end has reached the capacity end. It also handles the case where the argument points inside the sequence. It makes no random draw. Its growth callee computes a new capacity and tail-transfers to a storage-reallocation routine (not audited; assumed to be a draw-free storage service).

- Server run: `0x0CD507–0x0CD83A`, append calls at `0x0CD51E` through `0x0CD836`, at 18-byte spacing.
- Client run: `0x193A97–0x193DCA`, append calls at `0x193AAE` through `0x193DC6`.

Inside the run, the only instruction that is not part of the store-and-append pattern is one 32-bit zero store to world `+0xB8`, placed before the first value is stored. It does not involve the list.

**Unconditional.** No branch anywhere in the constructor targets an address at or after the start of the run (server constructor `0x0C8570–0x0CD85A`, 6045 instructions; client `0x18EB00–0x193DEA`, 6044 instructions). The constructor has exactly one return, which follows the run. Every construction that completes normally therefore runs all 45 appends, in the order below, with no condition, loop, configuration read, seed or random draw involved. Abnormal termination through an exception during construction is not addressed.

**No earlier filling.** Between the zeroing in 4.1 and the run, the register holding the list's address is used only to reload itself, never as a receiver or argument. No other constructor instruction uses displacement `0x88`, `0x8C` or `0x90` on the world. The list therefore holds exactly the 45 values once construction completes.

### 4.3 Complete contents

Every element is an unsigned 32-bit value. List order (index 0 first), decimal with hex in parentheses:

| Index | Value | Index | Value | Index | Value |
| --- | --- | --- | --- | --- | --- |
| 0 | 35 (`0x23`) | 15 | 105 (`0x69`) | 30 | 86 (`0x56`) |
| 1 | 87 (`0x57`) | 16 | 19 (`0x13`) | 31 | 99 (`0x63`) |
| 2 | 60 (`0x3C`) | 17 | 40 (`0x28`) | 32 | 106 (`0x6A`) |
| 3 | 55 (`0x37`) | 18 | 37 (`0x25`) | 33 | 63 (`0x3F`) |
| 4 | 34 (`0x22`) | 19 | 38 (`0x26`) | 34 | 66 (`0x42`) |
| 5 | 23 (`0x17`) | 20 | 39 (`0x27`) | 35 | 64 (`0x40`) |
| 6 | 22 (`0x16`) | 21 | 92 (`0x5C`) | 36 | 65 (`0x41`) |
| 7 | 30 (`0x1E`) | 22 | 93 (`0x5D`) | 37 | 74 (`0x4A`) |
| 8 | 33 (`0x21`) | 23 | 56 (`0x38`) | 38 | 36 (`0x24`) |
| 9 | 98 (`0x62`) | 24 | 90 (`0x5A`) | 39 | 57 (`0x39`) |
| 10 | 25 (`0x19`) | 25 | 91 (`0x5B`) | 40 | 58 (`0x3A`) |
| 11 | 53 (`0x35`) | 26 | 50 (`0x32`) | 41 | 59 (`0x3B`) |
| 12 | 67 (`0x43`) | 27 | 27 (`0x1B`) | 42 | 61 (`0x3D`) |
| 13 | 102 (`0x66`) | 28 | 75 (`0x4B`) | 43 | 62 (`0x3E`) |
| 14 | 104 (`0x68`) | 29 | 26 (`0x1A`) | 44 | 88 (`0x58`) |

The same sequence as one line: 35, 87, 60, 55, 34, 23, 22, 30, 33, 98, 25, 53, 67, 102, 104, 105, 19, 40, 37, 38, 39, 92, 93, 56, 90, 91, 50, 27, 75, 26, 86, 99, 106, 63, 66, 64, 65, 74, 36, 57, 58, 59, 61, 62, 88.

Count check: the table has indices 0 through 44, which is 45 entries. The line above also has 45 values, and both images have 45 append calls in the run. All 45 values are distinct, the smallest is 19 and the largest is 106, and they sum to 2750. Every value is below 256, so its low byte, which is what the producer uses, equals the value itself, and the three upper bytes of each element are zero.

The image provides no text naming these values. No game meaning is asserted.

### 4.4 Later changes

No routine was found that appends to, removes from, clears, overwrites or reorders this list after construction (see section 1 for the method and section 8 for its limit). The routines that reference the world's `+0x88`, `+0x8C` or `+0x90` through the world receiver chain of section 3 are only the constructor, the destructor and the readers in section 6. The contents are fixed for the life of the world object.

### 4.5 Release (world destruction)

- Server destructor body `0x0CD940` (the deleting destructor `0x0CE540`, the first entry of the world's method table, calls it). If the begin pointer is nonzero, it calls one imported routine with the list's address, releases the storage, and zeroes all three pointers (`0x0CDD2D–0x0CDD62`). The identity of that import was not resolved; the storage is released right after it in any case.
- Client destructor body `0x193ED0` (called by the deleting destructor `0x1946C0`, the first entry of method table `0x31A794`). It does the same at `0x1942BD–0x1942F8`. The three zeroing stores and the stack adjustment at `0x1942E2–0x1942F8` are missing from the listing and were read from the image.

## 5. Same-offset references shown not to be this list

These routines use displacements `0x88`/`0x8C` but belong to other object types or other members. They are listed so they are not mistaken for writers or readers of the world list.

- Server `0x0E28E0` appends to a 32-bit sequence at `+0x88` of its first argument (call at `0x0E76E1`). Its only caller is `0x118630` (at `0x11D452`), and that argument is not the world. Server `0x0D4C20` and `0x148E00` walk a sequence at `+0x88` whose elements they dereference as object pointers (`0x0D4C20` gets its object from a lookup routine; `0x148E00` destroys each element through its method table). Client counterparts of the same patterns: `0x1AF170`, `0x19ED30` and `0x224390`. Some of the 24 listing-uncovered client byte matches fall inside `0x224390`.
- Server `0x12E9E0` copies `+0x80` through `+0x94` between two objects. Its only caller allocates the destination with `0xA0` bytes, so the destination is not the world. The client counterpart is `0x208FF0`.
- Server `0x0C5A60`, `0x1308B0`, `0x0A1C00`, `0x0AA5E0`, `0x0AF360`, `0x0B60E0`, `0x0B6BF0`, `0x06AAB0`, `0x001140`, `0x0E2110`, `0x148B60`, `0x022910`, `0x0237B0` and `0x0272D0` treat `+0x88`/`+0x8C` as signed 64-bit coordinate halves, counters, flag words, linked-list heads or freshly zeroed members of their own objects. None of them is reached through the world receiver chain of section 3. Their receivers were not each traced further (section 8).

## 6. Readers of the list

### 6.1 World-list item producer (both images)

Client `0x1F79E0–0x1F7A5D`, server `0x12B3F0–0x12B46D`, identical instruction sequences. The count is computed as (end minus begin) shifted right arithmetically by 2. One draw is made through the random import. The index is the remainder of unsigned 32-bit division of the draw by the count, and the low byte of the element at that index becomes the item's subtype byte. With the contents above, the count is always 45, so the subtype is element number (draw mod 45) of the section 4.3 sequence. The empty-list fault noted in the earlier contract cannot occur on any receiver reached after construction completes.

### 6.2 Client second reader `0x0E4BD0`

This routine has no server counterpart: the server's displacement enumeration has no other count-and-index use of this list. It is called only from `0x0882E0` (at `0x088799`), whose first argument is `+0x2E4` of `0x0882E0`'s receiver. `0x0882E0` is called from `0x088EE0` (at `0x0906D5` and `0x096A4F`) with the same containing object that holds the world member at `+0x2E4`, so this reader receives the world of section 3.2. It reads the list at two sites, both with the same selection rule as the producer: one draw through the random import (`0x2FC468`), count as in 6.1, and an index equal to the unsigned remainder of the draw divided by the count. Unlike the producer, it keeps the whole 32-bit element and passes it as an argument to `0x194C80`. What `0x194C80` does with it is not covered here.

- Site A, `0x0E4CFD–0x0E4D1F`: reached when `0x0E4BD0`'s second argument, compared as a 32-bit value, is not 4 and equals 3.
- Site B, `0x0E5078–0x0E509B`: on the path that starts at `0x0E4EB1`. That path is reached when the second argument is neither 4 nor 3, and also from the argument-4 branch when either of its two entry checks fails. The path first makes one draw and reduces it by signed remainder modulo 5. Values above 4 jump straight to site B; values 0 to 4 dispatch through a five-entry table at `0x0E52DC`. The table cases were seen to rejoin before site B but were not individually audited, so whether every case reaches site B is left open. The random routine used at site B is called through a register loaded from the same import slot at `0x0E4EEA`. That register is not rewritten in between.

The listing range `0x0E4BD0–0x0E52DB` matched the image; its uncovered bytes are stack adjustments and two short release tails.

### 6.3 No other reader

No other routine in either image was found that indexes or walks the world's `+0x88` list through the world receiver chain of section 3. The bound of this statement is given in section 8.

## 7. Client and server agreement

Each image was established independently from its own bytes.

- **Receiver.** In both images the producer's receiver is the world object built by the image's world constructor. In the server, that object is a separately allocated `0x80029C`-byte block. In the client, it is the `+0x2E4` member of a larger containing object.
- **List members.** Both world objects keep the list at `+0x88`, `+0x8C` and `+0x90`.
- **Population.** Both constructors zero the list early and fill it with one unconditional run of 45 appends at the end of construction.
- **Contents.** The two runs were compared element by element. All 45 indices hold the same 32-bit value in both images: index 0 is 35 in both, index 1 is 87 in both, and so on through index 44, which is 88 in both. The comparison holds for both extraction methods (listing and raw bytes).
- **Release.** Both destructors release the list and zero its three pointers.
- **Producer.** The producer routines are identical apart from absolute addresses.
- **Difference.** The client has one more reader (6.2) with no server counterpart. It uses the same list and the same selection reduction, but it consumes the whole 32-bit element and serves a purpose outside this contract.

## 8. Unresolved and limits

- Game meaning of the 45 values: not asserted. The image provides no naming text for them.
- No exhaustive receiver trace: no proof covers every routine in either image. Section 5 lists the same-offset references shown, or classified by their access pattern, to belong to other objects, and the routines in its last group were not traced to their receivers one by one. The "no later writer" and "no other reader" statements rest on that classification together with the byte-scan cross-check of section 1.
- Some uncovered client bytes: the 24 client byte matches not covered by the listing were attributed by location, not disassembled one by one.
- Release import: the identity of the import called in the destructors before the list is released was not resolved.
- Untraced callees: the storage-reallocation routine reached by the append's growth callee was not audited.
- Client reader details: in 6.2, the full reach of the client reader's five-way dispatch and what `0x194C80` does with the element are not established.
- Construction count: how often the world object is constructed during one run of the program is not established. Every completed construction yields the same 45-element sequence.
- Out of scope: exceptions during construction, and concurrency, are not addressed.
