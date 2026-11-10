# Alpha fatal descriptor-tail lifecycle

Status: FINAL / STABLE / HELD. This original-only contract closes the concrete descriptor destruction reached by the fatal regional-completion tail loop in client RVA 0x195A60, the meaning and verified producer of the cell's +0xA0 boundary, and selected construction, activation, rebuilding and backing-storage paths. It distinguishes that destruction from a separate path which also removes instantiated creatures. It does not certify every possible writer of the pointer vector or every surrounding world lifecycle.

## Image and evidence identity

The evidence root is identified relatively as `disassembled alpha cube world`. All evidence paths below are relative to that root. The supported original image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base 0x00400000. All addresses below are RVAs within that image.

Original native instructions, original RTTI bytes and individually identified function files under `decompiled/client/functions` were inspected. `decompiled/client/assembly.txt` was searched only as original image evidence. Original routine 0x1E4850 has a decompilation-timeout notice; its scoped findings use native instructions rather than an invented missing high-level body. No earlier held document, reconstruction, implementation or runtime material was read.

C denotes the cell object returned by original spatial helper 0x034D10. S denotes a descriptor pointer stored in C's +0x18 vector. A denotes a separately instantiated creature. T denotes the fatal target whose coordinates select the completion area. W denotes the world. These labels do not assume that S and A have the same allocation or lifetime.

## Concrete pointed type and virtual entry

The verified tail producers allocate 0x10F0-byte descriptor objects and construct them through RVA 0x1ADB20. That constructor installs the vtable at image RVA 0x31DBEC. The original complete-object locator at 0x3481DC identifies the type descriptor at 0x3679E8; its original RTTI name is `.?AUSpawn@cube@@`. Thus `cube::Spawn` is an image-established descriptor identity, not a name inferred from reconstruction.

The first function pointer in that vtable is RVA 0x1ADDB0. This is the virtual method reached when the fatal loop invokes the first vtable entry of an ordinary S constructed by that path. The call supplies S as receiver and a four-byte argument equal to 1. Entry 0x1ADDB0 first invokes descriptor destructor body 0x1ADCA0. It then tests the low bit of its argument and, because the argument is 1, deallocates S's own allocation through the original scalar-deallocation thunk at 0x28D66E. The native return value is the former receiver pointer; the fatal caller does not use it.

This closes the virtual identity for the observed concrete descriptor. It is not an arbitrary game action callback on that path. A vtable replacement, a different concrete type inserted by an unclosed writer, corrupt pointer or duplicate ownership is outside this identity guarantee. The two producer families inspected below use this same constructor; an exhaustive inventory of every possible vector writer is not claimed.

Evidence: `decompiled/client/functions/005adb20.c`, `005adca0.c`, `005addb0.c`; native 0x1ADB20, 0x1ADCA0–0x1ADDA2 and 0x1ADDB0–0x1ADDCE; original RTTI/vtable bytes at the addresses above. The fatal virtual invocation itself is at 0x19660B.

## Cell pointer vector and the +0xA0 boundary

C+0x18 is the four-byte pointer to the beginning of a pointer vector; C+0x1C is its logical end and C+0x20 its capacity end. Each element is a four-byte S pointer. The count used here is the signed four-byte pointer difference between end and beginning divided by 4. C+0xA0 is a four-byte element index, not a byte offset, vector capacity or creature count.

Cell constructor 0x2240F0 initially zeros the vector members and C+0xA0. In the original successful new-cell construction path within 0x1E4850, native 0x1EDD53–0x1EDD64 obtains the current descriptor-vector count through 0x087F50, stores that count at C+0xA0, and then calls tail producer/rebuilder 0x1D9460 with C. Descriptors already present at this boundary form the retained prefix. Entries added afterward form the removable tail. The fatal completion loop does not move +0xA0 to the new vector end and does not turn it into a counter of removed objects.

The same new-cell path allocates a 0xC8-byte cell and invokes constructor 0x2240F0 at 0x1E495F–0x1E4984 after finding the requested cell absent. The final cell-pointer publication at 0x1EE05A stores C into its world block's cell-pointer array. Those sites establish cell storage and publication separately from creature-map insertion. Full generation between them, other cell-loading routes and every later boundary producer remain outside this contract.

Under the ordinary valid invariant, +0xA0 lies between zero and the current pointer-vector count. If it is zero, the whole vector is a tail for these consumers. The fatal code does not validate or clamp a malformed boundary before using its signed index. Out-of-range, negative or overflowed values therefore do not have a safe prefix-retention guarantee.

Evidence: `decompiled/client/functions/006240f0.c`, `00487f50.c`; native new-cell and boundary/publication sites in 0x1E4850; the original fatal reads at 0x1965E7–0x1965FD. The actual spatial lookup's coordinate and allocation admission is bounded to its original helper; this document does not redefine all cell lookup routes.

## Verified tail construction and rebuilding

Rebuilder 0x1D9460 acts only when W's four-byte member +0xA4 is nonzero. It begins with the existing tail at C+0xA0. For each old descriptor, it first searches W's creature map using the descriptor's complete eight-byte identifier at S+0x48. When that key resolves to a nonnull creature A, the rebuilder invokes A's first virtual method with argument 1, then obtains the matching-key range and erases it from W's creature map. Only afterward does it invoke the descriptor's own first virtual method with argument 1. It then truncates C's descriptor vector back to +0xA0.

This is an explicit creature-removal path. It is separate from fatal completion, whose descriptor-tail loop performs no such creature lookup, creature virtual call or world-map erasure. The creature virtual destructor's complete transitive work is not reconstructed here; its invocation and the following map-node removal are established boundaries.

After that old-tail cleanup, the rebuilder can allocate a new 0x10F0-byte S through 0x1ADB20 on its admitted regional-data route. It populates descriptor state and content, creates optional descriptor-associated behavior, assigns an eight-byte identifier using cell coordinates and append position, and appends S to C+0x18. In this route descriptor preparation includes calls to 0x03F7C0 and 0x1C7730 before the pointer is appended. Those preparation helpers and item generators can consume randomness; this is not the destruction path's draw count.

If that regional-data route is unavailable, the rebuilder calls fallback generator 0x1D6040. Its examined creation sites also allocate 0x10F0-byte objects through 0x1ADB20. Its final tail pass starts at C+0xA0, assigns complete identifiers to the added entries using cell coordinates and their vector positions, and invokes the same preparation helpers on each. The fallback has additional terrain, actor and generation predicates whose complete meanings are not closed here. Its presence does not prove that a fatal completion always or immediately recreates a tail.

Another original caller, 0x1D9AD0, invokes 0x1D9460 for nonnull cell pointers in its admitted world-block pass. That caller requires W byte +0xB4 equal to zero, W+0xA4 nonzero and a nonnull block, but its full scheduling/reset policy is outside this audit. Therefore deleting a tail is not, by itself, evidence of a permanent prohibition on later descriptor creation.

Evidence: `decompiled/client/functions/005d9460.c`, native 0x1D9494–0x1D954A and its later allocation/append route; `005d6040.c`, especially its 0x1D703F/0x1D7583 constructor calls and final tail preparation; `005d9ad0.c`, native call at 0x1DA1D9. Matching-key range and erasure helpers are `00609280.c` and `0067eb10.c`. A null descriptor in the rebuilder's initial ID lookup is not protected by the later null check before descriptor destruction; malformed pointer slots are not made safe by this contract.

## Fatal completion removal order

Within the admitted completion route of 0x195A60, the regional event is appended before descriptor-tail removal. The routine then visits the cell area selected by dividing T's signed +0x1B0 and +0x1B4 coordinates by 8 toward zero and multiplying the integer quotients by 8. Under unchanged coordinates and ordinary bounds, it visits eight first-coordinate values in ascending order, with eight second-coordinate values ascending inside each first-coordinate visit.

For each nonnull C, it starts at the signed index C+0xA0 and walks to the current pointer-vector end. A null element skips the virtual invocation. A nonnull ordinary S reaches the concrete destruction sequence above. The vector beginning and end are reloaded after each invocation. Once the tail walk finishes, the code rereads C+0xA0 and calls vector-resize helper 0x00D9D0 with that count.

For valid ordinary shrinkage, this resize changes the vector's logical end to the retained-prefix boundary. It does not free the vector's backing allocation, reduce capacity, clear every discarded pointer slot, invoke descriptor destructors again, or erase C from the world's cell-pointer array. Discarded storage can still contain old pointer bytes beyond the logical end; those bytes are not live vector entries. The boundary member +0xA0 is unchanged by this fatal operation.

The first coordinate's initial base is computed before traversal. The second base is recomputed for each outer row, and coordinate upper limits are recomputed from T during traversal. No immutable target-coordinate copy is maintained across virtual invocations. For the verified ordinary Spawn destructor, however, the game-side destruction chain described below does not write T, creature coordinates, cell coordinates or world mappings. With valid disjoint ownership and unchanged external state, this concrete path therefore does not supply a game callback that changes the area bounds while the loop runs. That conclusion is conditional on the established concrete type and valid ownership; it does not extend to corrupt pointers, another unproved vtable target or allocator failure/reentrancy.

Evidence: native 0x19657E–0x196685 and original `decompiled/client/functions/0040d9d0.c`. The descriptor invocation is 0x19660B; vector resize is 0x19662A. The resize helper has a separate capacity/allocation/zero-fill path for expansion, so a malformed boundary cannot be described simply as tail removal.

## Descriptor-owned backing destruction

The complete concrete destructor body has the following established storage effects, in this order:

| Descriptor storage | Established ordinary destruction effect |
| --- | --- |
| Four-byte pointer S+0x109C | If nonnull, pass that pointer directly to scalar deallocation |
| Three-word storage range S+0x10BC/+0x10C0/+0x10C4 | If its first pointer is nonnull, deallocate it and zero all three four-byte members |
| Three-word storage range S+0x10AC/+0x10B0/+0x10B4 | Same conditional storage deallocation and three-member reset |
| Three-word storage range S+0x10A0/+0x10A4/+0x10A8 | Same conditional storage deallocation and three-member reset |
| Outer three-word range S+0x0F6C/+0x0F70/+0x0F74 | For a nonnull beginning pointer, destroy its inner storage records, free the outer allocation, then zero the outer triple |
| S's own allocation | Freed afterward by 0x1ADDB0 because argument 1 has its low bit set |

The inner-storage helper at 0x239CF0 traverses the outer range as 12-byte records. For each record with a nonnull first pointer it deallocates that backing pointer and zeroes that record's three four-byte members. It does not invoke a virtual method on the stored contents. The outer allocation is released after those inner backing allocations.

In particular, the first +0x109C operation is a direct storage deallocation, not a virtual destruction call on the pointed object. No recursive release of that pointed object's own children can be inferred from this call. That pointer is not explicitly set to zero before S itself is freed on the argument-1 route.

The concrete chain contains no world/creature lookup, hit application, map erasure, coordinate conversion, game virtual callback, direct random/seed call or output-event append. It reads and resets descriptor storage and invokes the native deallocation import. No direct write to a creature's health, the fatal target's coordinates, W's creature-map nodes or the world cell-pointer table appears in that chain. Imported allocator internals and invalid aliasing of supposedly owned allocations are outside the established game-side effects.

Evidence: complete native 0x1ADCA0–0x1ADDA2 and 0x239CF0–0x239D2D; `decompiled/client/functions/005adca0.c`, `005addb0.c`, `00639cf0.c`. The scalar-deallocation thunk at 0x28D66E jumps through the original deallocation IAT slot; this audit does not treat it as a hidden game callback.

## Creature activation is a separate ownership boundary

The world update at 0x20C510 traverses C's descriptor-pointer vector and separately admits descriptor activation. Its gates include descriptor timing, absence of an already-instantiated creature under S+0x48, descriptor state/marker checks and proximity to an eligible world creature. Full input collection and every numeric activation predicate are outside this lifecycle addendum; a stored descriptor is not proof of immediate activation.

On the verified successful creation route at 0x20FFBA–0x21000E, the world allocates a distinct 0x1E60-byte creature, invokes creature constructor 0x03B690 using the identifier at S+0x48, and inserts that new pointer in W's creature map under that identifier. It then copies descriptor configuration and cell/index associations into the creature. The descriptor is neither moved into the creature allocation nor removed from C merely because activation succeeds.

Several owned-data boundaries are explicit. Descriptor arrays at +0x10AC and +0x10BC are copied into creature members +0x13E8 and +0x13F8 through assignment helper 0x27A7B0; that helper copies contents into destination storage, allocating destination backing when required, rather than transferring the source pointer triple. The data rooted at S+0x0F6C is copied to creature+0x11DC through 0x04AFF0 and its nested container-assignment helpers. Their inspected paths assign/construct 12-byte inner vector records and allocate/copy their 0x11C-byte elements; they do not establish a transfer of ownership of S's outer/inner allocations to the creature. Item semantics and all invalid-alias cases are outside this storage observation.

When S+0x109C is nonnull, activation invokes that pointed object's virtual method at vtable offset +4 and stores the result in creature+0x13E4. It does not directly store S+0x109C there. For the concrete 0x0C-byte object constructed at 0x0DC2D0 in the examined tail generators, this second virtual entry is 0x0DC400: it allocates a new 0x0C-byte object and invokes copy constructor 0x0DC200, which creates a separate child list and invokes each child's own +4 virtual entry. The outer result is therefore a distinct allocation on the successful ordinary route. Exact concrete child identities, their copy semantics and failures remain unresolved. Other possible descriptor behavior types are not assigned this specific cloning contract without evidence.

Consequently the known fatal Spawn destruction is not a native instruction to destroy its corresponding instantiated creature. There is no creature-ID lookup or map removal in that fatal tail loop. The explicit actor deletion/map erasure described for rebuilder 0x1D9460 must not be silently added to fatal completion. Conversely, this audit does not certify every later user of a deleted descriptor index or every lifetime of cloned behavior children.

Evidence: native 0x20FBB1–0x20FC29, 0x20FFBA–0x210185 and 0x210208–0x2103BF; `decompiled/client/functions/0067a7b0.c`, `0044aff0.c`, `0044ad30.c`, `0044aa20.c`, `0044a410.c`, `00449cb0.c`, `0044a700.c`; behavior constructor/copy entries `004dc2d0.c`, `004dc400.c`, `004dc200.c`. The latter's virtual child-copy boundary remains explicit rather than being assumed to return distinct children.

## Final cell ownership and limits

The original cell destructor at 0x224390 traverses the entire C+0x18 vector, not just its tail, and invokes each nonnull element's first virtual method with argument 1. It later releases that vector's backing allocation. This supplies an independent ordinary ownership boundary for retained prefix descriptors as well as any surviving tail. The full cell destructor also manages unrelated arrays and objects; its complete world-removal caller and all their side effects are not closed here. The cell's own deleting wrapper at 0x224760 can subsequently free C when its flag low bit is set.

The supported conclusions require valid live cells, ordinary vector boundaries, the observed concrete descriptor type and unique ownership of released allocations. The audit does not prove that every possible deserialization, modification or uninspected generator stores only that type; that no descriptor pointer can be duplicated; that arbitrary callback/import replacement is safe; or that later regeneration is impossible. Spawned-creature deletion in rebuilding, fatal completion's descriptor-only destruction, and final cell teardown have different proved boundaries and are not interchangeable operations.

Evidence: `decompiled/client/functions/00624390.c`, `00624760.c`; native full-vector invocation at 0x224414–0x224432. Full world unload/reload, persistence, synchronization, action-child ownership, exceptional allocation behavior and every indirect writer remain explicit dependencies. All older held artifacts remain unopened and unchanged.
