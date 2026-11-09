# Alpha terrain chunk point-light records

Status: FINAL / STABLE / HELD. This bounded contract establishes the original resident-record layout, normal rebuild and selection procedure, numeric conversion boundary, selected source producers and terrain upload. It does not establish every source's creation/expiry rule or the shader's use of the uploaded constants.

The evidence root is identified relatively as `disassembled alpha cube world`. The supported image is `binary/Cube.exe`, 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, preferred image base `0x400000`. All routine addresses below are RVAs. Object offsets are byte offsets; native pointers are 32 bits. Only original executable bytes and individually identified original function/assembly exports were used. No reconstruction, other contract or runtime observation supplied facts.

## Resident layout and upload count

The terrain renderer is within the original routine at RVA `0x0AC260`. At `0x0B1BFA`–`0x0B1C0B` it supplies chunk `+0x78` and `+0x178` to upload helper RVA `0x048F10`.

| Chunk field | Native representation |
| --- | --- |
| `+0x78` through `+0x177` | Sixteen records, each four binary32 values in X, Y, Z, radius order; 16-byte stride |
| `+0x178` through `+0x237` | Sixteen RGB records, each three binary32 values; 12-byte stride |
| `+0x238` | Four-byte selected-record count, manipulated and compared as a signed integer in this producer |

The cache entries have stride `0x268`. Their array base is client `+0x2E0`, and their grid dimension is the four-byte client field `+0x2DC`. The ordinary selection procedure limits the resident count to 16.

Resident capacity and upload count differ. Native helper `0x048F10` first requests **10** four-float vectors from the position/radius pointer, using the device-object method at vtable byte offset `+0x178` and the destination value stored at helper object `+0x68`. It then packs all 16 source RGB triples into temporary four-float records, adding a positive-zero fourth lane, and requests **10** vectors from that temporary array using the destination at helper object `+0x6C`. The two native count arguments are immediate `0x0A`, not `0x10`.

Thus this terrain call supplies resident slots 0–9 to each upload, even though construction and selection support slots 0–15. The helper does not consult chunk `+0x238`, compact active records, normalize RGB, clamp radius/color or apply gamma conversion. It copies the binary32 components. The meaning and use of these constants inside each shader are outside this contract; no claim of 16 shader lights follows from resident capacity.

At this terrain site the renderer has acquired the chunk critical section at `+0x250` and skipped chunks whose tested geometry collection is empty before making the upload. The complete locking policy for array construction and all chunk users is not established here.

Sources: `binary/Cube.exe`, RVAs `0x048F10`–`0x048FBA`; `decompiled/client/functions/00448f10.c`; `decompiled/client/assembly.txt`, terrain RVAs `0x0B1BC6`–`0x0B1C0B` and cache construction/selection RVAs `0x0B0E90`–`0x0B1272`. Native bytes establish the decimal upload count despite the separate 16-iteration color-packing loop.

## Temporary candidates and priority

This renderer invocation starts its temporary candidate vector empty at `0x0AC3C0`–`0x0AC3D6`. Appending a candidate through RVA `0x0C11F0` advances the vector by 48 bytes but copies only the first 44 bytes of the candidate's meaningful fields.

| Candidate offset | Native representation and role |
| --- | --- |
| `+0x00`, `+0x08`, `+0x10` | Three signed 64-bit position components with scale 65,536 per coordinate unit |
| `+0x18` | Binary32 radius |
| `+0x1C`, `+0x20`, `+0x24` | Binary32 RGB |
| `+0x28` | Binary32 sorting key |
| `+0x2C` through `+0x2F` | Unassigned padding in the inspected append copies; no semantic value established |

No source pointer or source identifier is copied into this candidate format. The later resident arrays retain position/radius/color, without a source identity or sorting-key field. Consequently the inspected selection has no source-ID tie breaker or deduplication by source identity. Different branches may append separate records associated with the same object.

After source collection, the renderer sorts the candidate range at `0x0B0E84`, through RVA `0x0ABAE0`. The native comparisons use candidate `+0x28` in ascending order. Audited producer branches derive this key from squared distance to the fixed-point reference vector at client `+0x140`, rather than distance to each receiving chunk. Their arithmetic subtracts fixed-point vectors, multiplies each component difference by itself in 64-bit fixed arithmetic, divides each component product by 65,536 before adding the terms, converts the resulting signed value to binary32 and multiplies by binary32 1/65,536. It must not be replaced by an assumed higher-precision floating distance calculation. The full key construction for every source branch remains unclosed.

For up to 32 records the inspected sorting route uses insertion ordering with strict smaller-key movement, retaining insertion order for equal ordinary finite keys. Larger ranges use partitioning and a fallback path; the partition routine moves equal-key records, so there is no general stable equal-key ordering guarantee. This audit does not supply a complete output permutation for larger equal-key sets, nor an ordering contract for NaNs or malformed keys. Such cases remain a dependency where a capacity boundary makes tie order observable.

Sources: `decompiled/client/functions/004c11f0.c`; `004abae0.c`; `004aa580.c`; `004a9520.c`; `0043ac20.c`; `0043b4a0.c`; `004120f0.c`; and `decompiled/client/assembly.txt`, partition RVA `0x0AB190`, including equal-key movement at `0x0AB2E0`–`0x0AB39A`. The listed function filenames are all relative to `decompiled/client/functions/` unless otherwise qualified.

## Clearing and assigning chunk records

After sorting and before assignment, the renderer visits the cache's squared grid count. For each visited entry it resets count `+0x238` to zero, all 16 position/radius records to four positive zeros, and all 16 RGB records to three positive zeros. This clearing happens even for slots that will not receive a candidate. The normal claims here assume a valid positive grid dimension and ordinary nonoverflowing cache dimensions; corrupt dimensions and integer-overflow paths are not characterized.

Each candidate then determines an inclusive rectangle of prospective chunk coordinates from its X and Y position minus and plus its radius. Radius is first multiplied by binary32 65,536 with binary32 rounding, then passed to the native signed 64-bit conversion helper. For finite representable ordinary inputs that helper truncates toward zero. The converted radius is subtracted from or added to the signed 64-bit position component. Each endpoint is divided by 65,536 with signed truncation toward zero, then divided by 32 with signed truncation toward zero. These operations do not implement mathematical floor for negative positions.

Prospective cells are admitted only within the cache interval beginning at client integer coordinates `+0x2AC` and `+0x2B0`, with each upper endpoint excluded at origin plus grid dimension. Coordinates must also be at least zero and strictly below `0x80000` on both axes. The cache slot uses the admitted coordinates' remainders by grid dimension: the Y remainder selects the row and the X remainder selects its column, with entry stride `0x268`. X advances in the outer candidate-cell loop and Y in the inner loop.

A cell whose signed selected count is already at least 16 skips that candidate. A cell below 16 receives the candidate in its current count slot, then increments the count. Thus each cell retains the earliest qualifying candidates in shared sorted order. There is no second per-chunk distance sort, replacement of an earlier selected light by a later one, color-strength ranking or sphere-versus-chunk intersection test in this inspected assignment body. Selection uses the rectangle, cache/world bounds and capacity checks just described.

The stored position adds the fixed-point shifts at client `+0x1D8` and `+0x1E0` to candidate X and Y; Z receives a zero shift. Each resulting signed 64-bit component is converted to binary32 before multiplication by binary32 1/65,536. Radius is copied unchanged into the fourth lane. RGB is copied unchanged into the corresponding 12-byte color slot. The producer does not make the position relative to each individual chunk. The higher-level ownership and maintenance of those two client shift fields are not reconstructed here.

Unselected cleared slots remain all zero, including zero radius and zero RGB. The inspected upload sends its fixed 10-vector prefix regardless of selected count; there is no separate enabled flag in this boundary. This identifies the native default representation without asserting how a shader handles zero radius.

Sources: `binary/Cube.exe` and `decompiled/client/assembly.txt`, clear RVAs `0x0B0E90`–`0x0B0F82`, endpoint conversion `0x0B0FC0`–`0x0B10AA`, bounds/capacity `0x0B10E6`–`0x0B1187`, resident writes `0x0B118D`–`0x0B126C`, and cell advancement `0x0B1272`–`0x0B12A0`; numeric helpers RVAs `0x012220`, `0x0122E0`, `0x02F570`, `0x02C4A0`, `0x02C800`, `0x03B610` and `0x28D946`. Constants at image RVAs `0x2FCD94` and `0x2FCE7C` are binary32 1/65,536 and 65,536.

## Verified source records and bounded producer dependencies

The following are original-evidenced producer relationships, with numeric identities retained where their wider object meaning was not independently closed. This is not a complete catalog of every candidate-producing branch in the large renderer.

**Chunk-associated source list.** Before the common sort, the renderer traverses records reached through a chunk's list at `+0x240`. A source's byte at `+0x40` must contain bit 0, and the original view/distance helper at RVA `0x07F760` must admit its position at source `+0x10`, supplied with culling-radius argument 16 and the renderer's local view-distance input. The source has a four-byte kind at `+0x08` and RGB at `+0x34`–`+0x3C`. Its candidate radius is a binary32 cosine-driven value around 15 with amplitude 0.5. Kind 13 additionally scales RGB by one minus half the renderer's local day-related factor; other admitted kinds use the source RGB directly. The complete phase/day-factor derivation, source creation, source-list replacement and source removal rules are not supplied. These source nodes are distinct from the rebuilt per-chunk selected arrays.

**World-list records.** For records in the list at client `+0x2F8`, the inspected branch requires the four-byte kind at source `+0x60` to equal 1. It takes position from source `+0x18`, gives the candidate radius 16, and scales RGB 0.5, 0.5, 0.2 by source binary32 `+0x4C`. This branch appends at `0x0AF442`. Its source-kind meaning and source-side deletion conditions are not inferred from this producer.

**Fading light records.** The list at client `+0x800748` supplies signed 64-bit fixed position at source `+0x00`, binary32 RGB at `+0x18`, radius at `+0x24`, and signed four-byte values at `+0x28` and `+0x2C`. It converts those last values independently to binary32, divides the former by the latter, subtracts the quotient from 1 and replaces an ordinary negative result by zero. That factor scales RGB; radius is copied unchanged. There is no ordinary upper clamp, and no color-zero rejection before append at `0x0AF583`. A zero-color candidate may therefore still occupy a selected slot. This rendering branch does not expire or delete the source record. Time units, who advances the two integers, zero-denominator/NaN behavior and removal timing remain unclosed.

**Creature-associated records.** The renderer traverses the world creature tree at client `+0x2E8`, skips null pointers and requires positive ordinary finite health at creature `+0x16C` before its light branches. It can append multiple candidates for a creature. Original examples include resident buff type 9 producing radius 16 and RGB 0.6, 0.4, 0, and the four-byte creature kind `+0x64` equal to `0x75` producing radius 16 and RGB 0, 0.4, 0.8. Kinds `0x87`, `0x88`, `0x89`, `0x8A` respectively produce radius 16 with RGB 0.1/0.5/0.1, 0.1/0.1/0.5, 0.5/0.1/0.2 and 0.8/0.8/0.6. Position construction uses the creature's render-position/attachment fields, including `+0x1350`; this contract does not replace the separate attachment transforms with a simple universal creature-position rule.

Other creature branches inspect equipment/flags/actions and an item-color helper. The inspected helper at `0x0C6CC0` averages four-component results over qualifying 8-byte item subrecords: the signed count is at item `+0x114`, and the byte tested above 127 begins at item `+0x17`. The deeper color lookup at `0x0C7250`, every equipment/action admission, all color scaling and attachment construction are not fully audited. Their presence is not a complete item-light specification.

**Sector-object records.** Later collection scans original sector lists. For object kind 50, and separately kinds 51 or 65, the renderer constructs a position using object fixed position `+0x08` and a Z offset from the third binary32 size component at `+0x24`. Radius is 20. Kind 50 uses RGB 1.2, 1, 0.8; kinds 51/65 use 1.2, 0.8, 0.4; both multiply RGB by one minus the renderer's local day-related factor. The admission calls use the original view/distance helper, with culling-radius 20 for kind 50 and 32 for kinds 51/65. The complete sector scan bounds, object-kind names and lifetime/streaming producers remain unclosed.

These groups are encountered in the described renderer order: chunk-associated sources, the first world list, fading records, creature branches, then sector-object collection, before common sorting. Within a source collection the original container traversal and branch append order feed the sort. This does not imply a source-identity order or priority after sorting.

Sources: `decompiled/client/assembly.txt`, source-list region `0x0AC73C`–`0x0ACAA6`, world-list region `0x0AF334`–`0x0AF47C`, fading region `0x0AF486`–`0x0AF5C5`, creature region `0x0AF5C5`–`0x0B06EF`, and sector region `0x0B0860`–`0x0B0E42`; `decompiled/client/functions/00443f10.c` for buff lookup; `004c6cc0.c` for item-color aggregation; `0047f760.c` for the inspected admission helper. Decimal-looking RGB constants denote their original binary32 values; for example 1.2 is `0x3F99999A`, 0.8 is `0x3F4CCCCD`, 0.6 is `0x3F19999A`, 0.4 is `0x3ECCCCCD`, 0.2 is `0x3E4CCCCD`, and 0.1 is `0x3DCCCCCD`.

## Lifetime, random-draw boundary and limits

The candidate vector is invocation-local and its storage is freed on the ordinary renderer cleanup route at `0x0BBB6D`. Resident arrays belong to persistent cache entries. The normal path rebuilds their contents after candidate collection/sorting and before the terrain upload. There is no resident per-light age, expiration timer or independent incremental fade in the inspected assignment/upload boundary; source records determine any fade or animation before selection. Early renderer routes can leave before this rebuild, so this is a phase-reached contract, not a promise that every display frame refreshes the arrays.

The inspected common sort, clearing, candidate-to-chunk assignment and upload contain no random-number draw. The cached-source radius animation uses the native cosine route, not a newly drawn random value. This does not make the entire renderer or all source creation draw-free: the same large renderer has native random calls in earlier scene-effect work, and it accesses the random generator again after this terrain upload. Those effect branches and their exact conditional draw counts were not reconstructed here. No fixed extra draw per selected slot, per chunk, per RGB packing iteration or per uploaded vector is justified by this boundary.

The complete producer/lifetime audit remains open for source-list construction/removal, fading-record update/deletion, all creature/equipment/action/attachment branches, sector streaming, exceptional numeric states, cache allocation/eviction, all callers and all shader consumers. Larger-sort equal-key permutations and transitive random draws inside unaudited source helpers also remain explicit dependencies. None of those gaps changes the verified distinction between 16 resident selections and the helper's 10-vector uploads.

Sources: `decompiled/client/assembly.txt`, local vector initialization `0x0AC3C0`–`0x0AC3D6`, ordinary cleanup `0x0BBB66`–`0x0BBB6D`, earlier native random call sites within `0x0ACC0C`–`0x0AD3CB`, and later random-function access at `0x0B1F07`; `decompiled/client/functions/005fb860.c`. The standard random import is at image IAT RVA `0x2FC468`; cosine thunk RVA `0x28D7A4` reaches the original imported precise cosine routine.
