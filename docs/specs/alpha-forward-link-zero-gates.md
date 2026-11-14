Status: FINAL / STABLE / HELD
# Forward-link zero gates in the original world update

Scope: how numeric-zero forward identifiers are treated by the client world update routine at RVA 0x20C510, chiefly its section at RVA 0x20E419–0x20EB15, plus the selection step at 0x20C8E1–0x20CA55 that feeds it. Supported image: `binary/Cube.exe`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, image base 0x00400000. All addresses are RVAs; evidence paths are relative to the original Alpha evidence root. Evidence is the original image, its instruction listing `decompiled/client/assembly.txt` (whose bytes for every range cited here were compared with the image file and matched), and individually named original function exports. No other contract, reconstruction or replacement implementation was read.

Summary: the category-0x13 maintenance never looks up a numerically zero forward identifier directly; it treats zero like a failed lookup and recovers or creates. Every other category looks the identifier up even when it is zero, and can therefore directly zero the health of whatever non-null actor is stored under map key zero. The later all-creature pass also looks up forward identifiers with no zero guard. Zero and a missing map entry are therefore not the same input.

## Inputs

For a selected actor A:
- The forward identifier F is the pair of 32-bit words at A+0x11C8 (low) and A+0x11CC (high). Every explicit zero test ORs both words, so F counts as zero only when both words are zero; a zero low word with a nonzero high word is nonzero.
- A's own identifier is the separate eight bytes at A+0x08/+0x0C.
- A linked actor's reverse owner identifier is the eight bytes at +0x198/+0x19C.
- The category decision reads exactly one byte, A+0x1020.
- Initial selection reads exactly one byte, A+0x60 (the role byte), with no class or specialisation interpretation.

[Evidence: `decompiled/client/assembly.txt`, RVA 0x20CA3D–0x20CA50, 0x20E483–0x20E4A4, 0x20E512–0x20E52E.]

## Selection

The creature-map walk at the start of the routine skips entries whose actor pointer is null. Later in the same per-entry iteration, after earlier per-actor handling that is not covered here (that handling can itself write the actor's +0x16C and +0x6C fields), every actor whose role byte is zero is appended to a temporary collection at RVA 0x20CA49–0x20CA50. That selection does not demand a nonzero own identifier, a nonzero forward identifier or positive health. The category pass later iterates the saved pointers without re-reading the role byte. Whether those pointers are still valid by then is a separate lifetime question. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20C8E1–0x20C903, 0x20C9AB–0x20CA55, loop continuation 0x20CCE5–0x20CCFF; `decompiled/client/functions/004860b0.c`.]

## Gates in front of the category pass

The category pass is only reached when the world byte at world+0xB4 is zero. When that byte is nonzero, the routine instead runs an alternate map pass whose every exit jumps to RVA 0x20F942, skipping both the category pass and the later pass at 0x20EA07; no other branch targets 0x20EA07. On the zero route, all branches between 0x20DEBB and 0x20E419 stay inside that span and flow into 0x20E419. There, helper RVA 0x077220 is applied to world+0x94: it reads the 32-bit word at its argument+0x10 (world+0xA4) and answers true exactly when that word is zero. True jumps straight to 0x20EA07. Otherwise the selected collection is iterated; an empty collection also jumps to 0x20EA07. The broader purpose of the world+0x94 object is not named here, and its zero test is not evidence of a pause or elapsed-time condition. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20DDEB–0x20DE24, 0x20DE24–0x20DEB6, 0x20DEBB–0x20E419, 0x20E419–0x20E464; `decompiled/client/functions/00477220.c`.]

## The lookup

Lookup RVA 0x02F000 is called with the world in ECX and the address of an eight-byte key. It searches the creature map at world+0x04 through RVA 0x077D90 (ordered by signed high word, then unsigned low word, and requiring exact equality). It returns zero when no equal key is present; otherwise it returns the actor pointer stored in that entry, which can itself be null. Callers therefore cannot distinguish an absent key from a present key with a null value. Nothing in the lookup rejects key zero: an entry with key zero and a non-null actor resolves like any other. The actor's own identifier is not consulted. [Evidence: `binary/Cube.exe`, RVA 0x02F000–0x02F027; `decompiled/client/functions/0042f000.c`, `00477d90.c`; the world-pointer and map-pointer locals are set at RVA 0x20C543 and 0x20C558.]

## Category 0x13: guarded lookup, recovery, refresh, creation

First gate (RVA 0x20E492–0x20E4B4). F is tested for zero before any lookup. Zero skips the lookup entirely, so a map entry under key zero is never consulted here. Nonzero F is looked up; only a non-null result skips recovery. An absent key or a null value takes the recovery path exactly as a zero F does. The resolved actor's role, health and reverse identifier are not examined. [Evidence: `binary/Cube.exe`; `decompiled/client/assembly.txt`, RVA 0x20E492–0x20E4C7.]

Recovery (RVA 0x20E4BA–0x20E580). F is first cleared, low word then high word. The creature map is then walked in its own order. A candidate B qualifies when its actor pointer is non-null, its role byte is 5, and its reverse owner identifier equals A's own identifier word for word. Neither identifier is required to be nonzero, so a zero reverse identifier matches a zero A identifier; a zero F does not by itself let a candidate match a nonzero A identifier. The first qualifying B supplies its own identifier (B+0x08/+0x0C) to F, low word then high word; the map key under which B is stored is not what is copied. Because this is an enumeration rather than a keyed lookup, it can visit an actor stored under key zero. Conversely, if B's own identifier is zero, F stays zero even when B is stored under some other key. Agreement between own identifier and map key is not checked. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E4BA–0x20E580; map-entry access helper `decompiled/client/functions/00468c90.c`.]

Second gate (RVA 0x20E586–0x20E5B6). A local refresh marker is cleared and F is tested again. Zero skips the lookup once more. Nonzero F is looked up a second time and a null result skips the refresh. So an initially zero F that recovery replaced with a nonzero identifier can lead to a lookup, but an unrecovered F, or one recovered as zero, cannot reach key zero here. This second lookup is by the copied own identifier, which may differ from the key that actually holds B. [Evidence: `binary/Cube.exe`; `decompiled/client/assembly.txt`, RVA 0x20E586–0x20E5B6.]

Refresh (RVA 0x20E5BC–0x20E6B8). For a non-null second-lookup result, a refresh happens when A's snapshot word at +0x1D44 differs from A+0x1138, or when the item stored at A+0x1D48 differs from the item at A+0x1020 (tested through RVA 0x0688F0, which negates item equality RVA 0x02F4A0). The marker is set before the resolved actor is reset and rebuilt from A's item and identity fields; this rebuild ends with a call to helper RVA 0x1EE7A0 that passes A's own identifier and the resolved actor. This refresh is separate from the other-category clearing and health store. Whether the refresh ran or not, A+0x1D44 then receives A+0x1138 and the item at A+0x1020 is copied into A+0x1D48. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E5BC–0x20E6DA; RVA 0x0688F0–0x068905.]

Creation gates (RVA 0x20E6DF–0x20E70D). Creation is considered only after the snapshot updates. A set refresh marker prevents it. Otherwise F is read again and any nonzero word prevents it. With F still zero, the bit 0x10 of the byte at A+0x124 is tested: set prevents creation, clear admits it. The existence of a non-null actor under key zero does not prevent creation, and no lookup of zero F happens between these gates and creation. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E6DF–0x20E70D.]

Creation (RVA 0x20E70D–0x20E8FC). A candidate identifier starts at eight zero bytes. Every map entry with a non-null actor pointer is examined, and the candidate is replaced by any key that is strictly smaller under signed-high-word, unsigned-low-word order. One is then subtracted from the candidate with 64-bit wraparound. A zero key never lowers the zero start value, and the result is always nonzero under this arithmetic, including the wrap from the smallest signed 64-bit key to the largest. This identifier is written into A's forward field before the new 0x1E60-byte actor is allocated, constructed with it as its own identifier, and stored in the map under the same key. The new actor is then reset, receives A's own identifier as its reverse owner identifier and role byte 5, and is configured with kind, level, item value, position, a computed value at +0x16C and further behaviour data. What happens when allocation fails or the key collides is not closed here. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E70D–0x20E8FC.]

Owner helper (RVA 0x20E8E8–0x20E8FC and 0x1EE7A0). After creation, helper RVA 0x1EE7A0 is called with A's own identifier and the new actor. The helper performs its own map search for that identifier with no zero rejection, so a zero A identifier can look up key zero at this point even though both direct forward lookups were skipped. When that search finds a non-null owner, the helper writes the new actor's own identifier into the owner's forward field (after lowering the new actor's +0x190 level to the owner's when greater, writing the searched identifier as its reverse identifier, and rescaling its binary32 value at +0x178). It performs no health-zero store. Its nested configuration is not a world-wide health or identity guarantee. The same helper is also called at the end of the refresh described above. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E6A1–0x20E6B8, 0x20E8E8–0x20E8FC, 0x1EE7C8–0x1EEA19; `decompiled/client/functions/005ee7a0.c`.]

## Other categories: lookup first, clear afterwards

When A+0x1020 is not 0x13, the branch at RVA 0x20E906 passes the address of F straight to lookup RVA 0x02F000 with no zero test. A zero F therefore resolves whatever non-null actor B is stored under key zero, regardless of B's own or reverse identifiers. [Evidence: `binary/Cube.exe`; `decompiled/client/assembly.txt`, RVA 0x20E483–0x20E48C, 0x20E906–0x20E91B.]

For a non-null B, the collection beginning at B+0x1178 is walked. The first element whose first byte equals 7 ends this branch at once: F is not cleared and no health is written. No other property of that element (scalar, duration, embedded identifiers) is examined. This can leave a zero F unchanged after a successful key-zero lookup. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E921–0x20E993.]

If the lookup returned null, or B holds no such selector-7 element, A's low forward word is cleared and then its high word. The saved lookup result is then tested; when non-null, the 32-bit zero pattern is written to B+0x16C (the health field) directly, without the generic hit or fatal handling. No positive-health, role, own-identifier or reverse-identifier test precedes the write, and nothing excludes B being A itself. A zero F can therefore zero the health of the actor under key zero, A included if A is stored there, whenever no selector-7 element intervenes. [Evidence: `binary/Cube.exe`; `decompiled/client/assembly.txt`, RVA 0x20E995–0x20E9C5.]

An absent key zero, or a key zero holding a null pointer, leads only to the clearing of F; the null result skips the health write. This branch has no recovery walk and creates no replacement actor, and neither A's own identifier nor B's reverse identifier influences whether zero is looked up. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20E906–0x20E9CD.]

## Later pass over all creatures

The pass at RVA 0x20EA07 runs whether or not the category pass ran, provided the world+0xB4 zero route was taken. It walks the creature map, skips null actor pointers, and looks up each traversed actor's current forward identifier at RVA 0x20EA65 without a zero test. That lookup precedes both the check that the traversed actor is the designated actor at world+0xB8 and the category check, so a still-zero F can resolve key zero here. When the traversed actor is the designated actor, the result is non-null, and the designated actor's category byte is 0x13, the result's 16-bit value at +0x190 is copied to the traversed actor's +0x1030 and its 32-bit value at +0x194 to +0x1024. This step writes no health. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20EA07–0x20EAA9.]

The same pass then handles traversed actors whose current role byte is 5. Their reverse owner identifier is looked up with no zero guard, so a zero reverse identifier can resolve key zero. If a non-null owner is found and the owner's forward identifier differs from this role-5 actor's own identifier, the role-5 actor is reset, its +0x16C health is set to zero, its role byte becomes 3, its 32-bit field at +0x6C becomes 5000 (0x1388), and its reverse identifier is cleared, in that order. This changes the role-5 actor's health, not the owner's. A null owner leaves the role-5 actor untouched. A selected actor that still has role zero does not enter this step. [Evidence: `decompiled/client/assembly.txt`, RVA 0x20EAAF–0x20EB0B.]

| Route, F numerically zero | Direct lookup of key zero | Immediate effect on that route |
| --- | --- | --- |
| Category 0x13, first gate | Skipped | F cleared, recovery walk |
| Category 0x13, second gate, F still zero | Skipped | Snapshot updates, then creation gates |
| Other categories | Performed | Selector-7 element keeps F; otherwise F cleared and a non-null result's health zeroed |
| Later all-creature pass | Performed | Designated category-0x13 actor may copy the resolved actor's +0x190/+0x194 values; no health write in this step |

## Limits

Numeric zero, null actor pointers, absent entries, a zero own identifier and a zero reverse identifier remain five distinct inputs. The integer tests and copies described here draw no random numbers. Creation and the helper calls invoke nested allocation, appearance, numeric and behaviour code whose effects and random-draw order are outside scope. This contract fixes the local branch, read and store order for valid native containers and readable retained pointers. It does not settle concurrency, aliasing of one actor under several keys, final health for the frame, later lifecycle outcomes, every upstream world gate, or any server counterpart.

## Re-certification (2026-11-14)

Re-checked by a source-side author against the original client image and listing (listing bytes compared with the file for all ranges cited) and the named exports. The Infinite-Cubes reconstruction was not consulted.

Confirmed:
- Field offsets and widths: F at +0x11C8/+0x11CC with OR zero tests; own identifier +0x08/+0x0C; reverse identifier +0x198/+0x19C; category byte +0x1020; role byte +0x60; health +0x16C; selector-7 walk at B+0x1178.
- Role-zero selection into a temporary collection; null entries skipped.
- world+0xB4 routing to 0x20F942; helper 0x077220 tests world+0xA4 for zero; empty selection skips to 0x20EA07.
- Lookup 0x02F000 semantics (world+0x04 map, null for absent or null-valued entries, no zero rejection).
- Category-0x13 first and second gates, recovery order and matching rule, marker handling, snapshot updates, creation gates (marker, F, bit 0x10 of +0x124), candidate-key arithmetic, write of the new identifier before allocation, reverse identifier and role 5.
- Helper 0x1EE7A0 owner search with no zero rejection and its write of the new actor's identifier to the owner's forward field.
- Other-category direct lookup, selector-7 retention, clearing order and unguarded health store.
- Later-pass lookup order and the +0x190/+0x194 copy; role-5 reverse lookup without zero guard.

Corrected:
- Role-5 mismatch handling omitted one write: the actor's +0x6C word is also set to 5000 (0x1388) between the role change and the reverse-identifier clear.
- The later pass was described as acting when "A is the designated actor"; it acts on each traversed creature, of which only the designated one at world+0xB8 receives the copy.
- The refresh condition is now stated precisely (snapshot word mismatch or item inequality via 0x0688F0/0x02F4A0), and the refresh's own call to helper 0x1EE7A0 is now recorded.
- The selection paragraph now notes that earlier handling in the same iteration can write +0x16C and +0x6C before the role test.
- Prose rewritten in independent wording.

Unresolved:
- Purpose of the world+0x94 object and the world+0xB4 byte; meaning of the selector-7 resident elements.
- Allocation failure or key collision during creation; full nested configuration and random-draw order.
- The earlier per-actor handling in the selection loop (role-3 and flag 0x2000 paths), concurrency, multi-key aliasing, server counterpart.
