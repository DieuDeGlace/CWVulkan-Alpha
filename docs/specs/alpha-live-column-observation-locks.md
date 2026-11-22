# Live terrain column reads in the Alpha local client

Author status **FINAL / STABLE / HELD**.

A current zone pointer, a valid column allocation and a consistent set of lighting bytes are three different observations. The original client uses two world critical sections across these operations. Either section can hold off ordinary zone departure once the observer has acquired it and found a resident zone. Neither section alone excludes all the traced writes to an already resident column.

## Which world and which locks

The world used by terrain meshing is embedded at client +0x2E4. The mesh builder selects this instance at RVA 0x09DAF7. It is also the instance passed to zone generation and zone removal at RVAs 0x06ACCB and 0x06AE1D. A neighboring call uses a distinct instance at client +0x800D44. The offsets below are relative to the mesh builder's world.

The constructor initializes distinct Windows critical sections at world +0x8000C0 and +0x8000D8. Their addresses relative to the client are +0x8003A4 and +0x8003BC respectively. The C0 enter and leave wrappers are RVAs 0x201CB0 and 0x201E90. The D8 pair is 0x19C480 and 0x201EA0. See RVAs 0x059F43–0x059F57 and 0x1933A4–0x1933BC.

## Zone and column lifetime

Lookup at RVA 0x034D10 traverses the primary world's region table at +0xBC and the region's zone-pointer table at +0x10018. Lookup at 0x0347A0 resolves the 0x20-byte column in the zone's array at +0xA8. Neither lookup takes a lock or retains the result. The stored column has a base elevation at +0x10, a voxel-buffer pointer at +0x18 and a record count at +0x1C. The addressed stored record is four bytes wide. An out-of-range accessor result can point at a shared fallback. Such a value provides no evidence of a stored voxel at that elevation. See RVA 0x19AAF0.

On ordinary zone departure the worker calls RVA 0x1A4890. The remover resolves the existing table entry, enters D8 and then C0, clears the entry, leaves C0 and then D8, and subsequently saves conditionally and calls the deleting destructor. Region departure enters C0 and then D8, clears the parent table entry, releases the sections in reverse order and destroys the region. These are opposite nesting orders. Acquiring a section after fetching a pointer leaves a gap in which that pointer can retire. Holding a section across lookup and use blocks the respective unlink on these paths. See RVAs 0x1A4929–0x1A496B, 0x1A4800 and 0x224390.

An edit can replace the voxel allocation under a still-resident zone. The resizing routine copies existing records, frees the old buffer and updates the pointer, base and count. The queued client block-edit path holds D8 around edits and the following cleanup while the surrounding update holds C0. See RVAs 0x050C50, 0x04E700, 0x04E7A0 and 0x08F25F–0x08F3D9. A saved voxel-buffer address alone does not identify a live allocation after this scope ends.

## Writers to the four bytes

The mesh worker releases its C0 scope at RVA 0x0693DD before dispatch to the builder at 0x069580. Inside the builder the primary world enters D8 at 0x09DB0B, may relight resident terrain at 0x09DB84, and releases D8 at 0x0A08E8. A later D8 interval at 0x0A0CD2–0x0A0F24 samples decoration lighting. The renderer slot's critical section at slot +0x250 starts after that interval and governs publication of slot data.

The main update enters the same world's C0 at 0x08902F and leaves at 0x09D2EA. Under that outer scope it can call the simulation at 0x08CF2E and 0x08CF49. A terrain-changing branch there can remove a block from a resident column at 0x20EECD. It can also relight a rectangle from resident columns at 0x20F071 if world +0xB8 is zero and the preceding branch conditions pass. Its D8 acquisition at 0x20F084 follows that relight and covers subsequent cleanup. Another image sequence removes at 0x213DD0 and conditionally relights at 0x213FC0 with the same +0xB8 test. The world constructor clears +0xB8, and a client actor setup route stores its actor pointer there at 0x070408. These conditional writes establish distinct synchronization scopes. They do not establish a relight on every local frame or for every block removal.

Initial generation has a different destination. At RVA 0x1EDE84 it passes the new zone explicitly to the relighter. The column accessor bounds that lookup to the supplied zone. The generator publishes the zone pointer at 0x1EE05A, after initial lighting. Thus a change from missing to resident has a separate cause from a later change in the contents of an existing column.

Byte +3 holds the type in its low five bits and also carries flags. The mesher uses bytes +0 through +2 as color components for ordinary colored terrain. In stored air and water, types 0 and 2, the relighter uses those same bytes for light. It seeds +1 and +2 from the top down, propagates light into +1, copies +1 into +2 after each sweep, and after sixteen sweeps copies +2 into +0 for the output rectangle. The propagation rectangle can be wider than the output rectangle. The relighter leaves other voxel types and the type/flag byte alone. See RVAs 0x19A0E0 and 0x09D910. An air or water record can therefore contain a previous output alongside scratch values from a later pass. A four-byte read alone cannot certify a single completed pass across a column, the phase of generation, or the light used in a presented frame.

## Consequence for a live observation

For an outside reader of an already published column, simultaneous exclusion of the traced C0 and D8 writers, from the first table lookup through completion of the copy, covers the described removals, queued edits and live light writes. A blocking acquisition of both in an arbitrary fixed order can conflict with the opposite order used by native code. Equivalently, a sample at a point where all relevant writers are independently known to be quiescent can preserve the same data. A thread stopped partway through a native write can preserve an intermediate phase. Separate copies of different columns at different times do not establish a common instant. The evidence gives no universal single-lock snapshot guarantee for every native writer, publication of previously absent zones, or world teardown.

Evidence is the original 32-bit Alpha client `Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, and its associated native disassembly. RVAs use image base 0x00400000. This contract describes the ordinary routes cited above and assigns no measured frequency to races or intermediate byte values.
