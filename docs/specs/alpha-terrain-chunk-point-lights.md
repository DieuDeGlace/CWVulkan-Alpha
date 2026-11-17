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

At this terrain site the renderer acquires the chunk critical section at `+0x250` before testing the geometry collection at chunk `+0x08`. The test skips the upload when the four-byte field at chunk `+0x0C` is zero. The complete locking policy for array construction and all chunk users is outside this scope.

Sources: `binary/Cube.exe`, RVAs `0x048F10`–`0x048FBA`, `0x077230`–`0x077238` and import slot `0x2FC070`; `decompiled/client/functions/00448f10.c`; `decompiled/client/assembly.txt`, terrain RVAs `0x0B1BC6`–`0x0B1C0B` and cache construction/selection RVAs `0x0B0E90`–`0x0B1272`.

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

After source collection, the renderer sorts the candidate range at `0x0B0E84`, through RVA `0x0ABAE0`. The native comparisons use candidate `+0x28` in ascending order. Audited producer branches derive this key from the fixed-point separation between the candidate and the reference vector at client `+0x140`. Each component difference is squared with a 64-bit product. Its low 64 bits are read as signed and divided by 65,536 with truncation toward zero. The three quotients are added in 64 bits. The resulting signed sum converts through x87 to binary32 before multiplication by binary32 1/65,536. The full key construction for every source branch remains open.

For up to 32 records the inspected sorting route uses insertion ordering with strict smaller-key movement, retaining insertion order for equal ordinary finite keys. Larger ranges use partitioning and a fallback path; the partition routine moves equal-key records, so there is no general stable equal-key ordering guarantee. This audit does not supply a complete output permutation for larger equal-key sets, nor an ordering contract for NaNs or malformed keys. Such cases remain a dependency where a capacity boundary makes tie order observable.

Sources: `decompiled/client/functions/004c11f0.c`, `004abae0.c`, `004aa580.c`, `004a9520.c`, `0043ac20.c`, `0043b4a0.c`, and `004120f0.c`; `binary/Cube.exe`, arithmetic helper RVAs `0x28D9E0` and `0x28DA90`; `decompiled/client/assembly.txt`, partition RVA `0x0AB190`, including equal-key movement at `0x0AB2E0`–`0x0AB39A`. Unqualified function filenames are relative to `decompiled/client/functions/`.

## Clearing and assigning chunk records

After sorting and before assignment, the renderer visits the cache's squared grid count. For each visited entry it resets count `+0x238` to zero, all 16 position/radius records to four positive zeros, and all 16 RGB records to three positive zeros. This clearing happens even for slots that will not receive a candidate. The normal claims here assume a valid positive grid dimension and ordinary nonoverflowing cache dimensions; corrupt dimensions and integer-overflow paths are not characterized.

Each candidate then determines an inclusive rectangle of prospective chunk coordinates from its X and Y position minus and plus its radius. Radius is first multiplied by binary32 65,536 with binary32 rounding, then passed to the native signed 64-bit conversion helper. For finite representable ordinary inputs that helper truncates toward zero. The converted radius is subtracted from or added to the signed 64-bit position component. Each endpoint is divided by 65,536 in signed 64-bit arithmetic with truncation toward zero. The renderer takes the low 32 bits of that quotient as a signed value and divides it by 32, again truncating toward zero.

Prospective cells are admitted only within the cache interval beginning at client integer coordinates `+0x2AC` and `+0x2B0`, with each upper endpoint excluded at origin plus grid dimension. Coordinates must also be at least zero and strictly below `0x80000` on both axes. The cache slot uses the admitted coordinates' remainders by grid dimension: the Y remainder selects the row and the X remainder selects its column, with entry stride `0x268`. X advances in the outer candidate-cell loop and Y in the inner loop.

A cell whose signed selected count is already at least 16 skips that candidate. A cell below 16 receives the candidate in its current count slot, then increments the count. Thus each cell retains the earliest qualifying candidates in shared sorted order. There is no second per-chunk distance sort, replacement of an earlier selected light by a later one, color-strength ranking or sphere-versus-chunk intersection test in this inspected assignment body. Selection uses the rectangle, cache/world bounds and capacity checks just described.

The stored position adds the fixed-point shifts at client `+0x1D8` and `+0x1E0` to candidate X and Y; Z receives a zero shift. Each resulting signed 64-bit component is converted to binary32 before multiplication by binary32 1/65,536. Radius is copied unchanged into the fourth lane. RGB is copied unchanged into the corresponding 12-byte color slot. The producer does not make the position relative to each individual chunk. The higher-level ownership and maintenance of those two client shift fields are not reconstructed here.

Unselected cleared slots remain all zero, including zero radius and zero RGB. The inspected upload sends its fixed 10-vector prefix regardless of selected count; there is no separate enabled flag in this boundary. This identifies the native default representation without asserting how a shader handles zero radius.

Sources: `binary/Cube.exe` and `decompiled/client/assembly.txt`, clear RVAs `0x0B0E90`–`0x0B0F82`, endpoint conversion `0x0B0FC0`–`0x0B10AA`, bounds/capacity `0x0B10E6`–`0x0B1187`, resident writes `0x0B118D`–`0x0B126C`, and cell advancement `0x0B1272`–`0x0B12A0`; numeric helpers RVAs `0x012220`, `0x0122E0`, `0x02F570`, `0x02C4A0`, `0x02C800`, `0x03B610` and `0x28D946`. Constants at image RVAs `0x2FCD94` and `0x2FCE7C` are binary32 1/65,536 and 65,536.

## Verified source records and bounded producer dependencies

The following are original-evidenced producer relationships, with numeric identities retained where their wider object meaning was not independently closed. This is not a complete catalog of every candidate-producing branch in the large renderer.

**Chunk-associated source list.** Before the common sort, the renderer traverses records reached through a chunk's list at `+0x240`. A source's byte at `+0x40` must contain bit 0, and the original view/distance helper at RVA `0x07F760` must admit its position at source `+0x10`, supplied with culling-radius argument 16 and the renderer's local view-distance input. The admitted position supplies the candidate position. The source has a four-byte kind at `+0x08` and RGB at `+0x34`–`+0x3C`.

The radius phase uses a counter initialized to zero for each traversed chunk list and increased by 33 for each record passing both admission tests, before calculating that record's radius. The signed four-byte field at client `+0x8003A0` converts to binary32 and is multiplied by binary32 `0.01` (`0x3C23D70A`). The counter converts to binary32 and is added to that product in binary32. The phase promotes to binary64 for the imported precise cosine call. Its result narrows to binary32, is multiplied by `0.5`, and has `15` added, in separate binary32 operations. The clock field's time unit and update rule are open.

Kind 13 scales RGB by one minus half the renderer's material-alpha time weight. Other admitted kinds copy the source RGB. The time weight starts by converting the signed four-byte clock at client `+0x800440` to binary32, doubling it, dividing by binary32 `86400000`, and subtracting one in binary32. The value promotes to binary64 for the imported precise power call with exponent four. Its result narrows to binary32 and is subtracted from one in binary32. The difference promotes for the second precise power call, with binary64 exponent five. The final result narrows to binary32. The kind-13 scalar uses separate binary32 multiplication by `0.5` and subtraction from one before multiplying the RGB channels. Source creation, replacement and removal remain open.

**World-list records.** For records in the list at client `+0x2F8`, the inspected branch requires the four-byte kind at source `+0x60` to equal 1. It takes position from source `+0x18`, gives the candidate radius 16, and scales RGB 0.5, 0.5, 0.2 by source binary32 `+0x4C`. This branch appends at `0x0AF442`. Its source-kind meaning and source-side deletion conditions are not inferred from this producer.

**Fading light records.** The list at client `+0x800748` supplies signed 64-bit fixed position at source `+0x00`, binary32 RGB at `+0x18`, radius at `+0x24`, and signed four-byte values at `+0x28` and `+0x2C`. It converts those last values independently to binary32, divides the former by the latter, subtracts the quotient from 1 and replaces an ordinary negative result by zero. That factor scales RGB; radius is copied unchanged. There is no ordinary upper clamp, and no color-zero rejection before append at `0x0AF583`. A zero-color candidate may therefore still occupy a selected slot. This rendering branch does not expire or delete the source record. Time units, who advances the two integers, zero-denominator/NaN behavior and removal timing remain unclosed.

**Creature-associated records.** The renderer traverses the world creature tree at client `+0x2E8` and skips null pointers. For finite health at creature `+0x16C`, the light branches are reached only when health is positive. Behavior for nonfinite health is outside this scope. The renderer can append multiple candidates for a creature. Original examples include resident buff type 9 producing radius 16 and RGB 0.6, 0.4, 0, and the four-byte creature kind `+0x64` equal to `0x75` producing radius 16 and RGB 0, 0.4, 0.8. Kinds `0x87`, `0x88`, `0x89`, `0x8A` respectively produce radius 16 with RGB 0.1/0.5/0.1, 0.1/0.1/0.5, 0.5/0.1/0.2 and 0.8/0.8/0.6. Position construction uses the creature's render-position and attachment fields, including `+0x1350`. The complete attachment transforms are outside this scope.

Other creature branches inspect equipment/flags/actions and an item-color helper. The inspected helper at `0x0C6CC0` averages four-component results over qualifying 8-byte item subrecords: the signed count is at item `+0x114`, and the byte tested above 127 begins at item `+0x17`. The deeper color lookup at `0x0C7250`, every equipment/action admission, all color scaling and attachment construction are not fully audited. Their presence is not a complete item-light specification.

**Sector-object records.** Later collection scans original sector lists. For object kind 50, and separately kinds 51 or 65, the candidate starts at object fixed position `+0x08` and adds a Z offset from the third binary32 size component, at `+0x2C` in the size record beginning at `+0x24`. The offset multiplies that component by binary32 65,536, then converts to signed fixed point through the same truncating conversion used for ordinary radii. Radius is 20. Kind 50 uses RGB 1.2, 1, 0.8. Kinds 51 and 65 use 1.2, 0.8, 0.4. All three scale RGB by one minus the material-alpha time weight, using binary32 subtraction and multiplication. The view/distance helper tests the Z-adjusted position with culling radius 20 for kind 50 and the base position with culling radius 32 for kinds 51 and 65. A separate collection at sector `+0x30` reaches the append at `0x0B0DCD`. Its full admission and color rules, the sector scan bounds, object-kind names and lifetime and streaming producers remain open.

These groups are encountered in the described renderer order: chunk-associated sources, the first world list, fading records, creature branches, then sector-object collection, before common sorting. Within a source collection the original container traversal and branch append order feed the sort. This does not imply a source-identity order or priority after sorting.

Sources: `decompiled/client/assembly.txt`, material-alpha time weight `0x0AC3E1`–`0x0AC44A`, source-list region `0x0AC73C`–`0x0ACAA6` and its continuation `0x0AD411`–`0x0AD434`, world-list region `0x0AF334`–`0x0AF47C`, fading region `0x0AF486`–`0x0AF5C5`, creature region `0x0AF5C5`–`0x0B06EF`, sector region `0x0B0860`–`0x0B0E42`, and conversion helper `0x02C460`; `decompiled/client/functions/00443f10.c`, `004c6cc0.c`, and `0047f760.c`; `binary/Cube.exe`, cosine and power import slots `0x2FC444` and `0x2FC454`, and data RVAs `0x2FCDA0`, `0x2FCE38`, `0x302AC4`, `0x302B50`, `0x302B58`, `0x345DA0`, `0x345DC0`, and `0x345E10`. Decimal RGB constants identify native binary32 values. The stored bits for 1.2, 0.8, 0.6, 0.4, 0.2 and 0.1 are `0x3F99999A`, `0x3F4CCCCD`, `0x3F19999A`, `0x3ECCCCCD`, `0x3E4CCCCD` and `0x3DCCCCCD`, respectively.

## Lifetime, random-draw boundary and limits

The candidate vector is invocation-local and its storage is freed on the ordinary renderer cleanup route at `0x0BBB6D`. Resident arrays belong to persistent cache entries. The normal path rebuilds their contents after candidate collection/sorting and before the terrain upload. There is no resident per-light age, expiration timer or independent incremental fade in the inspected assignment/upload boundary; source records determine any fade or animation before selection. Early renderer routes can leave before this rebuild, so this is a phase-reached contract, not a promise that every display frame refreshes the arrays.

The inspected common sort, clearing, candidate-to-chunk assignment and upload contain no random-number draw. The cached-source radius animation uses the native cosine route, not a newly drawn random value. This does not make the entire renderer or all source creation draw-free: the same large renderer has native random calls in earlier scene-effect work, and it accesses the random generator again after this terrain upload. Those effect branches and their exact conditional draw counts were not reconstructed here. No fixed extra draw per selected slot, per chunk, per RGB packing iteration or per uploaded vector is justified by this boundary.

The complete producer/lifetime audit remains open for source-list construction/removal, fading-record update/deletion, all creature/equipment/action/attachment branches, sector streaming, exceptional numeric states, cache allocation/eviction, all callers and all shader consumers. Larger-sort equal-key permutations and transitive random draws inside unaudited source helpers also remain explicit dependencies. None of those gaps changes the verified distinction between 16 resident selections and the helper's 10-vector uploads.

Sources: `decompiled/client/assembly.txt`, local vector initialization `0x0AC3C0`–`0x0AC3D6`, ordinary cleanup `0x0BBB66`–`0x0BBB6D`, earlier native random call sites within `0x0ACC0C`–`0x0AD3CB`, and later random-function access at `0x0B1F07`; `decompiled/client/functions/005fb860.c`. The standard random import is at image IAT RVA `0x2FC468`; cosine thunk RVA `0x28D7A4` reaches the original imported precise cosine routine.

## Source re-certification, 2026-11-16

The supported image hash and instruction bytes were checked against the original executable. The renderer interval `0x0AC260`–`0x0BBB6D` contains 20 direct calls to the candidate append helper. Those call sites cover one chunk-list producer, one world-list producer, one fading producer, fourteen creature branches and three sector branches. The counts refer to call sites in the image, with dynamic execution governed by the original loops and conditions.

The endpoint correction records the signed interpretation of the low 32 quotient bits at `0x02F570` and `0x0B0FF6`–`0x0B10AA`. The renderer's cached-source phase and shared time weight are traced at `0x0AC3E1`–`0x0AC44A` and `0x0AC758`–`0x0ACAA6`. The distinct sector culling positions follow from `0x0B0928`–`0x0B0954` and `0x0B0A83`–`0x0B0AAD`. The geometry-gated upload is tied to the `0x077230` field test and the byte-checked calls at `0x0B1BD1`–`0x0B1C0B`.
