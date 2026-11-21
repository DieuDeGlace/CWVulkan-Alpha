# Alpha terrain-mesh vertex accounting

**Author status — FINAL / STABLE / HELD**

## What the worker charges

The original Alpha client charges **four primary vertex records per emitted quad**, including liquid faces and the additional top face described below. Each record is eight bytes. A published group records its primary vertex count at group offset `0x10`. The builder sums those counts across the chunk's published groups and replaces the signed 32-bit count at mesh-slot offset `0x23C`. The worker sign-extends that field when adding it to the pass total. Rebuilding replaces the previous count, and resetting the slot sets the count to zero.

The reference is the original 3,885,568-byte x86 client `Cube.exe`, preferred image base `0x00400000`, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`. All addresses below are RVAs. The evidence comprises the original image, client disassembly, scoped routine decompilations and direct call records. Mesh construction begins at `0x09D910`. The publication and sum are at `0x0A1230–0x0A1398`, worker accumulation at `0x06945B–0x069474`, and reset at `0x086CD7`.

## Which faces contribute

The builder processes a 32-by-32 square of terrain columns. It chooses the current cell and each axial neighbor using the effective five-bit material type. Type 0 is empty, type 2 is liquid, and every other nonzero type takes the ordinary terrain path. For an ordinary or liquid cell it tests neighbors in the order negative X, positive X, negative Y, positive Y, below and above. Each decision belongs to the current cell.

| Current cell | Neighbor across the face | Quads charged |
| --- | --- | --- |
| Empty | Any | Zero |
| Liquid | Empty | One, liquid index stream |
| Liquid | Liquid or ordinary | Zero |
| Ordinary | Empty or liquid | One, ordinary index stream, plus the special case below |
| Ordinary | Ordinary, including a different ordinary type | Zero |
| Type 3, upward face only | Empty directly above | Two in total, one in each index stream |

At an ordinary/liquid interface only the ordinary side contributes a quad. A type-3 cell with liquid directly above still emits its ordinary top quad. The extra quad requires type 0 directly above. When that condition holds, the builder emits four extra upward-facing records to the common primary stream and six indices to the liquid index stream, then emits the ordinary top. Both quads use the same four geometric corners at the top elevation. The extra quad has its own lighting input and its own vertices. The numeric material ID and branch behavior establish this exception independently of any player-facing name for type 3.

The source chooses the index stream at `0x09EB34–0x09EBD7`. The six directional paths occupy `0x09EC8B–0x09F253`, `0x09F2FD–0x09F79E`, `0x09F7BC–0x09FAF6`, `0x09FB15–0x09FE5A`, `0x09FE7A–0x0A01B7`, and `0x0A01D5–0x0A0886`. The separate type-3 check, second neighbor sample and extra quad are at `0x0A01F9–0x0A056A`.

## Column bounds and implicit cells

For an available current column the emitter starts one elevation below the lowest base among that column and its available four horizontal neighbors. It stops before the larger of one and the current column's base plus stored record count. Current columns must be in the world and have their region and column storage available. Missing neighbor samples affect exposure through their effective type.

| Sample | Effective type for face tests |
| --- | --- |
| Missing sample or elevation below column base | Type 1, ordinary solid |
| Within storage | Low five bits of the stored type byte, subject to the next row |
| Stored type 0 at elevation zero or lower, with flag `0x40` clear | Type 2, liquid |
| At or above the end of storage | Type 2 at elevation zero or lower, otherwise type 0 |

Thus a stored empty cell at or below zero with flag `0x40` set stays empty. The implicit liquid and below-base solid can contribute faces even though there is no stored record of that material at the sampled elevation. The upper bound is exclusive even when it is raised to one. The column loop and bounds are at `0x09E0C7–0x09E377`, current-cell material resolution at `0x09E3B7–0x09E420`, and neighbor lookup at `0x02F7E0` with equivalent inlined X-neighbor checks. The fallback material bytes are initialized at `0x2F9720–0x2F979D` and `0x2F9B10–0x2F9B8D`.

## Records, indices and vertical groups

The primary stream is shared by ordinary and liquid terrain within each vertical group. The builder takes the current number of primary records as the base index for each new quad. It appends four new records, each with a byte for each local position component, one face-direction byte and four packed color bytes. It writes six 32-bit indices, forming two triangles over those four records. For example, the quad index appender uses corner numbers 0, 1, 2 and then 0, 2, 3 relative to the quad's base. Lighting changes the color values at the corners without changing the record count.

Adjacent quads receive fresh vertices even if their positions, material and colors coincide. An exposed vertical wall three terrain cells high therefore contributes three quads and twelve records for that wall. Adjacent coplanar tops also contribute separately. Interior face culling is the reduction in this path. There is no subsequent vertex welding, greedy rectangle merge or triangle expansion before the worker's count is taken. Evidence includes record packing at `0x066650`, single-record append at `0x086830`, the quad index appender at `0x06EEB0`, and the per-corner calls in each directional range above.

The builder sizes parallel group collections from the chunk's vertical bounds in bands of 250 elevations. Group membership is based on the cell elevation relative to the lower bound. All corners of a face stay in its cell's group, including a top corner on the next band's boundary. Grouping creates no extra faces at a band boundary. A group with primary records supplies a vertex buffer. At publication a group contributes only if its primary buffer pointer is non-null. For each such group, the primary count is the occupied byte span divided by eight. Its two index counts are stored separately as triangle counts at `0x14` and `0x18`. The worker adds neither triangle count, index storage, spare capacity nor the buffer-object size. The group setup is at `0x09E03B–0x09E084`, band selection at `0x09E380–0x09E3AA`, buffer preparation at `0x0A0AB0–0x0A0C7F`, and per-group publication at `0x0A1230–0x0A137A`.

## Decorations and accounting examples

After preparing the terrain buffers, this builder visits a separate region list of positioned records, filters them by chunk, samples terrain lighting, and copies matching records into a per-slot list at `0x248`. The renderer transfers that list to `0x240` and visits its entries separately. These records add zero to the worker vertex count. Geometry already represented by terrain cells follows the cell rules regardless of its visual purpose. The type-3 additional top is one example of a visual layer that does count. Evidence for the separate list is at `0x0A0CD2–0x0A0E97`, `0x0A114A`, `0x0AC631–0x0AC652` and `0x0AC73C–0x0AC765`.

The examples give the contributions of the indicated cells only. All unmentioned neighbors are effective type 0, all indicated cells are visited, and their groups are published. Horizontal adjacency is assumed unless a cell is explicitly described as above another.

| Cells | Ordinary quads | Liquid-stream quads | Primary records charged |
| --- | ---: | ---: | ---: |
| One type-1 cell | 6 | 0 | 24 |
| Two adjacent type-1 cells | 10 | 0 | 40 |
| One type-2 cell | 0 | 6 | 24 |
| Two adjacent type-2 cells | 0 | 10 | 40 |
| Adjacent type-1 and type-2 cells | 6 | 5 | 44 |
| One type-3 cell | 6 | 1 | 28 |
| Type-3 cell with a type-2 cell directly above | 6 | 5 | 44 |

These counts establish the retained terrain mesh on the successful publication path. Assigning a particular named plant, tree, prop or other decoration to terrain voxels or to a separate object-rendering path requires evidence for that object's producer. The behavior of failed graphics allocations and the vertex counts of independent model draws remain outside this contract.
