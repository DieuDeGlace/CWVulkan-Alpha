# Alpha spatial association-word production boundaries

## Supported evidence and result

The supported image is the original 32-bit `binary/Cube.exe` under the relative evidence identifier `disassembled alpha cube world`, preferred base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses in this document are RVAs. Evidence citations refer to that root's `decompiled/client/assembly.txt` and individually identified original files under `decompiled/client/functions/`.

This audit closes the association-word origin of the **directly appended spatial records** in all three requested generator bodies. Those records receive `0xFFFFFFFF` from their constructors/builders and retain it when appended. No first nondefault, zero or positive producer for spatial record `+0x178` was established. The result does not supply a missing association by assigning an array ordinal.

The field-focused inspection covered the complete original instruction bodies from `0x1AF170` through the return at `0x1BF3D2`, from `0x1CC510` through the return at `0x1D5F8C`, and from `0x1E4850` through the return at `0x1EE0B8`. The original listing contains respectively 61,916, 39,444 and 37,430 encoded instruction bytes for these bodies; those bytes were checked against the supported executable. The corresponding three function decompilations report timeouts and were not treated as completed behavioral evidence. This is a field-production audit across those bodies, not a specification of all terrain geometry or every nested generator.

Reconstruction material was used only for leads. Every affirmative relation below rests on original instructions; reconstruction field names and comments do not establish a producer.

## The independent storage being examined

Here, M denotes one spatial value record of extent `0x188` bytes. The relevant member is a full 32-bit word at `M+0x178`. The containing cell's value vector has its begin, end and capacity pointers at cell `+0x0C`, `+0x10` and `+0x14`; its elements are `0x188` bytes apart. This vector is distinct from the cell's four-byte descriptor-pointer vector beginning at `+0x18`.

The default association word has the raw value `0xFFFFFFFF`, interpreted as -1 by signed integer use. It is not a byte flag or a floating-point quantity. The following words at `+0x17C`, `+0x180` and `+0x184` are separate storage. A nearby kind, position, active byte or collection count is not an assignment to `+0x178`.

The constructor at `0x18EAA0` writes `+0x178=-1`, then `+0x17C=-1`, then `+0x180=-1`, then `+0x184=0`, at `0x18EACB–0x18EAE9`. This follows construction of the nested member beginning at `+0x48`. The nested constructor `0x03C020` ends its explicit member initialization at nested `+0x12C`, which is M `+0x174`; it does not supply the association word that follows. Evidence: `0058eaa0.c`, `0043c020.c`, and original instructions.

The three specialized builders `0x1BF5B0`, `0x1BFD70` and `0x1BFF10` also initialize the same four tail words to `-1,-1,-1,0`. Their coordinate, orientation, kind and size inputs do not replace `+0x178`. Their random kind/size choices occur after its default store and do not determine that word. Original stores begin at `0x1BF5EB`, `0x1BFDAB` and `0x1BFF4A`. Evidence: `005bf5b0.c`, `005bfd70.c`, `005bff10.c`.

Two further builders, `0x1BF7C0` and `0x1F6E20`, initialize their temporary record's same tail to `-1,-1,-1,0` and return a copied record through `0x18EA10`, at `0x1BFCC0` and `0x1F7379`. Their branching shape, orientation and position choices do not replace the temporary's association word. Evidence: `005bf7c0.c`, `005f6e20.c`, and original temporary construction and final copy instructions.

## Direct output families of the requested generators

The counts below are distinct native call sites, not the number of records emitted by one invocation. Branches and loops determine which sites execute and how often. They are included to identify the audited production coverage without claiming a fixed output count.

| Generator | Direct append sites using `0x1A0E00` | Origin of the appended association word |
|---|---:|---|
| `0x1AF170` | 56 | 21 sites use records initialized by `0x18EAA0`; 12 use `0x1BFF10`; 8 use `0x1BFD70`; 8 use `0x1BF5B0`; 7 use `0x1BF7C0`. Every family supplies -1. |
| `0x1CC510` | 9 | 5 sites use `0x18EAA0` records; 4 use `0x1F6E20` results. Every family supplies -1. |
| `0x1E4850` | 3 | All three use `0x18EAA0` records and supply -1. |

For `0x1AF170`, the plain-constructor append sites include `0x1B062B`, `0x1B21F2`, `0x1B23FB`, `0x1B8579`, the four sites from `0x1B874D` through `0x1B8AD4`, `0x1B8C95`, the four from `0x1B8E39` through `0x1B91C0`, the four from `0x1B939F` through `0x1B9726`, and the four from `0x1BC34F` through `0x1BC759`. Repeated appends of a reused temporary do not reinitialize or replace its association word with the growing vector count. The specialized-builder families occur at the other direct append sites between `0x1B083A` and `0x1BA229`. Original instructions across the complete `0x1AF170` body establish the temporary origins, intervening member accesses and append arguments.

For `0x1CC510`, the plain-constructor outputs are appended at `0x1D0D68`, `0x1D0F0C`, `0x1D10B8`, `0x1D1256` and `0x1D1C01`. The first four temporary records receive kind 4; the fifth receives kind 7. The four other outputs follow calls to `0x1F6E20` and append at `0x1D2565`, `0x1D28CA`, `0x1D2C2F` and `0x1D2F9B`. There is also a constructor call at `0x1D0B80` followed immediately by temporary destruction at `0x1D0B8F`; that temporary is not an additional direct append or a nondefault producer.

For `0x1E4850`, construction/append pairs are `0x1E9BCF/0x1E9E15`, `0x1EA891/0x1EA9B6`, and `0x1EBE02/0x1EBEFB`. The first record receives kind `0x2D`, the second kind 0, and the third kind `0x32` or `0x33`. Even the kind-`0x2D` construction retains association -1: its later kind, orientation and dimension stores are not association stores. Its kind assignment is at `0x1E9DCB`, followed by an orientation draw and the copy append. No count or ordinal is inserted in its tail at that boundary.

The audit followed the full-record temporary addresses as well as their interior member addresses and the direct append sources. Merely finding no instruction spelling `+0x178` would not establish these results: most constructor stores occur in callees, and temporary-relative offsets differ from record-relative offsets. Conversely, stores to other objects or other local lifetimes at similar numeric offsets were not classified as spatial associations.

## Intervening operations and existing-record access

Where the specialized objects pass through placement helper `0x1F4DA0`, that helper's direct record mutations concern the vertical position words at `+0x18/+0x1C`. It does not fill the association word. Calls receiving only a record's position or dimension subobject likewise do not turn their component index into `M+0x178`. These statements concern valid, nonoverlapping records and cell structures; corrupt or deliberately overlapping arguments are excluded. Evidence: `005f4da0.c` and the original record-argument paths in the three generator bodies.

The generators also address already appended records through helper `0x04B560`, which selects an element using the `0x188`-byte extent. In `0x1CC510`, the later pass `0x1D30CB–0x1D3605` uses this access for kind checks, inventory work, size/position reads and an active-byte change. In particular, the kind-10 route writes byte `+0x30=2` at `0x1D3192`; that is not an association assignment. The inventory resize and add helpers receive record `+0x48`; their direct fixed members and owned allocation operations do not supply the subsequent `+0x178` word. Evidence: `0044b560.c`, `00487380.c`, `0046ebe0.c`, and original pass instructions.

The indexed-record pass in `0x1AF170` beginning at `0x1B979A` reads record kinds and constructs references in separate collections. It does not write an association into the indexed spatial record. This distinction matters when an ordinary array ordinal is visible among the pass's arguments: visibility of that ordinal is not a tail-member producer.

## Copy, append and lifetime

Full record copying at `0x18EA10` reads source `+0x178` at `0x18EA58` and stores that complete word to destination `+0x178` at `0x18EA61`. It subsequently copies the separate following tail words. There is no special case for zero, positive values or `0xFFFFFFFF`; supplied bits survive unchanged. A nondefault source would therefore propagate, but this copy does not explain its initial origin. Evidence: `0058ea10.c` and original instructions.

The prefix copy helper `0x068670`, also reached during record copying, assigns selected members only within offsets `+0x00` through `+0x44`, including the one-byte active state. It neither reaches the association word nor guarantees a copy of every padding byte in the prefix. Evidence: `00468670.c`.

Append helper `0x1A0E00` preserves a source record's array position when that source already belongs to the destination vector and capacity growth moves the storage. That ordinal is used to recover the source address. It is not written into the appended record. The append copy precedes the end-pointer advance by `0x188` at `0x1A0E9A`. Evidence: `005a0e00.c`, especially original `0x1A0E00–0x1A0E9A`.

Capacity replacement at `0x1949D0` copy-constructs the existing range through `0x18E8E0` before destroying old records and replacing the vector pointers. Records retain their association bits in element order. These operations do not rebase an association according to either the new allocation address or an element ordinal. Evidence: `005949d0.c`, `0058e8e0.c`, `0058d970.c`.

Temporary destruction at `0x193E80` handles the nested allocation beginning at record `+0x48`. It contains no association reset. Destruction ends the temporary's lifetime; it does not establish a meaningful surviving reference in released storage. Evidence: `00593e80.c`. Allocation, exception and invalid-overlap behavior are not generalized into successful-generation guarantees.

## Reconstruction and separate ordinal publication

The cell reader `0x04CDB0` has a record-state restoration phase at `0x04D3D0–0x04D429`. It accepts that phase only when the stored signed record count equals the current spatial-vector count and is positive. Its per-record payload is one byte, copied to `M+0x30`; it advances through records by `0x188`. That phase does not read a full spatial record or reconstruct `+0x178`. If a byte is unavailable, the local buffer reader advances its cursor to the available end without that byte's destination store. The matching writer's phase at `0x04E66E–0x04E6CB` emits the count and each `+0x30` byte, not the association tail. Evidence: `0044cdb0.c`, `0044e340.c`, and original instructions.

The large generator conditionally calls this reader at `0x1EDE46`, after its direct spatial record generation. This closes that particular restoration phase as a source of active-state changes, not as a first nondefault association producer. It does not specify the complete file format or all earlier reader phases.

Later, `0x1EDF67–0x1EE048` enumerates spatial records and publishes entries into separate per-position collections. The entry constructor `0x1C38A0` receives a spatial-record pointer and three 32-bit values: the cell's two coordinates and the current spatial-vector ordinal. The constructor writes four words to its own entry. List insertion `0x1F4B50`, through `0x208A20`, copies those four entry words to a new list node. It does not dereference the stored record pointer to assign `M+0x178`. Thus an actual spatial-array ordinal is supplied here, but its destination is a separate reference entry. Evidence: `005c38a0.c`, `005f4b50.c`, `00608a20.c`, and original `0x1EDFF0–0x1EE02D`.

The containing cell pointer is stored into its owner's cell slot at `0x1EE05A`, after this indexing work. That pointer publication is not a copy or reset of the spatial association word. Broader synchronization and cache replacement ownership are outside this field-production contract.

## Remaining origin gap

These results rule out the audited direct output families, their demonstrated placement/interior operations, copy/relocation work, the specified active-state restoration phase and separate spatial-ordinal reference publication as first nondefault association producers. They do not prove that every possible path in the executable preserves -1 forever.

In particular, the complete transitive behavior of all other generator callees, virtual callbacks, scripts, external-memory inputs and every possible alias to an existing spatial record has not been closed. The enclosing generators call additional world-building routines, including `0x1F0CE0`, `0x1AD320` and `0x1DC6E0`; direct output coverage of their callers is not a substitute for complete transitive closure of those routines and all their dependencies. No server-image counterpart is inferred.

No original evidence in this audit establishes a first assignment of zero or a positive index to spatial `+0x178`, its source identity, or the conditions for such an assignment. A consumer comparing that word with a creature's backing ordinal cannot supply those missing producer facts. Nor do the separate ordinal entries above justify populating it. The behavior beyond the established -1 and raw-copy boundaries remains unresolved.

Status: FINAL / STABLE / HELD after complete author readback.
