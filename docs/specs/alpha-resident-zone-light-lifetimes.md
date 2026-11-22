# Resident zones, lighting and object lifetimes in the Alpha local client

Author status **FINAL / STABLE / HELD**.

The ordinary terrain light and mesh path keeps the resident zone, its spawn descriptors and its indexed world objects in place. A mesh build can change a lighting field on an existing decoration. A block edit can remove individual decorations that have lost support. Departure from the resident set releases the zone and its owned records.

## Formation and ownership

The world resolves a zone coordinate through a region and then a 64 by 64 table of zone pointers within that region. The resident pointer is the entry at region offset 0x10018 for that coordinate. The zone owns a 256 by 256 array of terrain columns at offset 0xA8. Independent collections include the decoration list at +0x04 with its count at +0x08, an array of 0x188-byte indexed object records beginning at +0x0C, and an array of pointers to spawn descriptors beginning at +0x18. These offsets refer to the zone itself. See RVAs 0x034D10, 0x2240F0 and 0x224390.

The world worker requests generation at RVA 0x06ACD1. After preparing region-level dependencies, the generator looks up the target zone again. An occupied entry branches past construction. On a miss it allocates the 0xC8-byte zone, populates its terrain and records, can apply saved changes, computes lighting against that explicit zone, and finally installs the pointer in the region table. An example spawn allocation is 0x10F0 bytes and calls the spawn constructor. The initial lighting call precedes publication of the new zone. See RVAs 0x1E4850, 0x1E4952–0x1E497F, 0x1EADB9–0x1EADCC, 0x1EDE46–0x1EDE84 and 0x1EE05A.

## Ordinary relighting and mesh publication

The mesh worker checks resident terrain before invoking the builder. The builder calls the lighting routine with a terrain rectangle and no explicit new zone. For this route the lighting routine resolves existing columns and updates light channels on eligible four-byte voxel records. Its propagation and output writes use the existing voxel storage. This route preserves the world table entry and the three zone-owned collections above. See RVAs 0x0690A0, 0x06F490, 0x09D910, 0x09DB84, 0x19A0E0 and 0x19AAF0.

The builder also looks up the resident zone, walks its decoration list and selects the records whose horizontal positions fall in the chunk being built. For each match it samples the block at the decoration. Type 13 yields 255. Air and water (types 0 and 2) yield the first voxel byte with a minimum of 5. Other types yield zero. The builder stores the result as a float on the original list node at +0x30 (decoration payload +0x28), then copies the decoration into a temporary render list. The resident node keeps its identity and position. RVAs 0x0A0D05–0x0A0E9A include the write at 0x0A0E49 and the copy at 0x0A0E59.

Publication acts on a renderer cache slot. Under its lock the builder publishes the decoration copies and pending geometry and updates the slot's chunk information. The renderer later transfers pending geometry and copies pending decorations from slot +0x248 to the active collection at +0x240. These operations can replace the slot's previous view of a chunk. The world table entry, spawn descriptor pointers, indexed object array and resident decoration nodes have separate ownership. See RVAs 0x0A114A–0x0A1394 and 0x0AC58A–0x0AC64B. Cache invalidation at RVA 0x086BA0 likewise operates on the render slot.

## Edits and departures

A queued block edit on the client updates the addressed terrain column. After the batch, the client passes the affected horizontal bounds to a cleanup routine. That routine walks the decoration list of each resident zone covered by those bounds. It selects nodes whose flag byte at +0x40 has mask 0x02 set and whose sample one voxel below the decoration has type 0. It collects the selected nodes before unlinking and freeing them. Thus losing support can remove particular decorations while the zone and other records remain resident. This scan operates at zone granularity within the affected bounds. See RVAs 0x08F2D0–0x08F3D5, 0x071920, 0x04E7A0 and 0x1A5CB0.

A separate queued object update validates the resident zone and object index, assigns selected fields on that indexed 0x188-byte record and clears zone byte +0x74. When applying saved data during zone formation, the native reader can replay block edits and performs a corresponding unsupported-decoration check for a positive serialized edit count. It also restores two words at spawn descriptor offsets +0x38 and +0x3C and one byte at indexed object offset +0x30 when each collection's serialized count matches its generated count and input is available. Those field updates retain the generated records. See RVAs 0x068670, 0x08F4AB–0x08F4F6, 0x04CDB0 and 0x04D2D4–0x04D41D.

The resident-set worker examines the zone's own coordinates. In its ordinary retention scan, any tracked coordinate with squared zone distance below 16 keeps the zone. A zone selected for departure reaches the removal routine at RVA 0x06AE23. That routine clears the region's zone pointer under the world locks, releases those locks, takes the conditional save path where applicable, then dispatches the zone's deleting destructor. The destructor visits the owned spawn pointers, destroys the indexed object storage and decoration list, and releases terrain storage. A later miss for those coordinates can construct a fresh zone and apply available saved state. See RVAs 0x06ADE0–0x06AE36, 0x1A4890, 0x1A5050, 0x224390 and 0x224760.

## Evidence and limits

Source-side static evidence comes from the original 32-bit Alpha client image `Cube.exe` (3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`) and its corresponding client disassembly. Addresses are relative to image base 0x00400000. The field and ownership findings cover these ordinary routes. A spawn descriptor and a live actor are distinct objects. Actor activation, movement, death and replication, the complete set of object edit messages, and preservation of every gameplay field across departure and reload remain outside the evidence established here.
