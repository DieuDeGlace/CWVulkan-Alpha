# Alpha water and wet-ground material contract

Status: FINAL / STABLE / HELD. Source-side native evidence contract, dated 2026-11-07. This document describes the bounded material, terrain-production, contact and surface-mesh facts below. It does not claim native runtime acceptance or prescribe replacement-engine changes.

## Evidence boundary and image

Original evidence paths in this document are relative to the `disassembled alpha cube world` evidence root. The supported image is `binary/Cube.exe`: 3,885,568 bytes, SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`, PE machine `0x014C`, PE32 optional-header magic `0x010B`, preferred image base `0x00400000`, image size `0x003BC000`, COFF timestamp `0x51EA955E`. RVAs below are relative to that preferred base. These are client-image facts; server equivalence is not established here.

The author read the assigned source-author policy first, inventoried filenames, and used original per-function decompilation, original assembly and the original image. Individually selected Infinite-Cubes Alpha-owned block, terrain and movement files supplied search leads only. Every positive material/contact/mesh claim below was checked against original Alpha evidence. No replacement engine, exporter, SDK, tests, captures, logs, existing specifications, semantic audits, subsystem leads or implementation history were read. Reconstruction names are not native identifiers.

## Material representation and classification

One stored voxel occupies four bytes. Bytes `+0`, `+1`, `+2` carry color channels; byte `+3` carries a five-bit material identifier and three upper bits. The native material accessor returns only the low five bits, using mask `0x1F`. The native solidity predicate rejects material 0 and material 2 and accepts the other values. Therefore upper bits do not turn material 3 into liquid or remove its solidity. [N1]

| Low-five-bit value | Evidence-supported meaning in this contract | Native solid predicate | Liquid contact scan |
| --- | --- | --- | --- |
| 0 | Empty/open material | False | Does not set the liquid state |
| 2 | Water volume | False | Sets the liquid state when intersected by the tested creature box, subject to the creature-class gate below |
| 3 | Wet solid ground, including water-like shallow surfaces | True | Does not set the liquid state |
| 6 and 12 | Examples of other solid terrain materials produced by the native terrain path | True | Do not set the liquid state |

“Wet ground” is a behavioral description: material 3 is selected independently by the terrain producer, receives a special surface face, participates in wet foot contact and selects a distinct footstep sound family. It is not inferred from its color or from a generic “underwater” flag. The present evidence does not establish official native textual names for these numeric materials. [N1–N5]

The natural-surface selector can emit material 12 and sets upper bit `0x20` on its final output. The main terrain producer also writes material 6 for solid fill. The lake and bank paths described below explicitly construct material 3 instead. Color alone cannot distinguish those cases: the material-3 paths obtain their color from the same native terrain-color routine used elsewhere for solid terrain. This contract does not assign a general semantic name to bit `0x20`. [N3, N6]

## Water volume versus bottom and bank production

The native terrain producer at RVA `0x1E4850` contains two independently identifiable wet-terrain paths. They are described by their outputs and numeric record gates, without importing reconstruction labels. [N3]

For the lake-producing path, the current record's 32-bit kind at offset `+0x18` must equal 4. The horizontal shape-distance result must be strictly below `0.25`. The upper processed height is the integer conversion, truncating toward zero, of the record's floating height at `+0x14` minus `25.0`. Processing descends through the column base, inclusive. At each processed cell, an existing solid material becomes material 3 with native terrain color. An existing material 0 or 2 instead receives the default water record. That record has color bytes `(255,255,255)` and packed material byte `0x82`: material 2 plus upper bit `0x80`. Thus lake bottom/solid ground and open water volume are explicitly different producer results. [N3, N7]

For the river-producing pass, the tested distance-field result is scaled by `50.0` and subtracted from `1.0`; a negative result skips the column. A second influence-field result above the single-precision constant represented by `0x3F733333` (approximately `0.95`) also skips it. Negative terrain height is clamped to zero. The resulting height is truncated to an integer and rounded down to a multiple of five, establishing the terraced water level. [N3]

The pass computes a depth/bank profile from the height within that five-block interval. The profile uses thresholds and scales `0.5`, `2.0`, `4.0` and `5.0`; it can produce an interval of material-2 cells through the terraced water level, or no such interval near a bank. The explicit liquid writes construct material 2 with red and green zero and a profile-dependent blue channel. They skip existing cells with upper bit `0x40` set. [N3]

The bank candidate starts at the terraced level and can be raised by the negative part of the profile. If the candidate is at or above that level, its existing low-five-bit material is not 2, and its upper bit `0x40` is clear, the producer writes material 3 with native terrain color. On that successful bank path it then takes a random draw; remainder zero modulo 200 records the location for a later terrain/vegetation stage. The later stage and the identity of its vegetation are outside this contract. The producer's protected-cell check and material-2 exclusion are significant: “near the river” does not mean every surface cell becomes liquid. [N3]

The above facts establish producer distinctions and constants, not the complete river distance field, profile formula, color function, regional shaping or generation order. Those remain separate reconstruction contracts.

## Upper bits and implicit water

Bit `0x40` has a verified protection/exterior-water role in this scope. The integer-coordinate reader substitutes water for material-0 cells at `z <= 0` only when that bit is clear. Above the stored column it returns default water at `z <= 0` and default empty material at `z > 0`. A missing column or a coordinate below its stored base returns the solid default instead. Consequently a consumer of native reads can observe water without an explicit stored material-2 cell. [N8]

The world writer substitutes its `0x82` water default for a write at `z <= 0` when the supplied packed byte has no bits in mask `0x5F`. The column writer also preserves an existing `0x80` bit: a replacement with material 0 is refused, while a nonzero-material replacement receives the new four-byte record with `0x80` retained. This makes that bit persistent provenance/state for a stored cell; it is not itself the liquid classification. A material-3 cell retaining `0x80` is still material 3 under the native material, solidity and contact tests. [N9]

The three inspected water defaults, used by the reader, world writer and lake producer, are each initialized to color `(255,255,255)` and packed byte `0x82`. This contract does not claim that every explicit material-2 write carries `0x80`: the river path explicitly supplies byte `2`. [N7, N9]

## Collision and contact gates

The movement routine at RVA `0x20C510` uses the native solid predicate while testing terrain obstruction. In one tested branch, a separate creature flag can additionally require water rather than generic non-solid space. That exception does not make material 3 non-solid. [N1, N4]

During state recomputation, the creature's flags word at offset `+0x5C` is rebuilt. The contact region is the inclusive integer-cell box obtained from position minus and plus half the creature dimensions. The conversion helper divides fixed coordinates by `65,536`, truncates toward zero, and subtracts another one for a negative input; exact negative integers therefore also move down one cell. [N4, N10]

The first box scan sets flag `0x10` if any tested cell has a material other than 2. A following scan, skipped when the creature-class byte at `+0x60` equals 6, sets flag `0x02` if any tested cell is material 2. It also records the maximum encountered water-cell top (`z + 1`) for the following buoyancy calculation. Material 3 does not satisfy either material-2 equality. [N4]

The subsequent ground check runs only while liquid flag `0x02` is clear. It samples the horizontal footprint at a height `0.2` below the feet, applies the same negative-coordinate conversion, and sets ground flag `0x01` when the solid predicate succeeds. A separate equality to material 3 at that foot plane sets a local wet-contact result. That wet result is also set by the liquid scan. Therefore native wet contact combines two cases that remain distinct in the movement flags: water volume and feet on wet solid ground. [N4]

One verified use of that combined result is the creature-class-5 path: if wet contact is present, a found owner's float at offset `+0x1198` rises by elapsed seconds times `0.05`, capped at `1.0`. Separately, the walking sound path samples half a block below the feet; material 3 selects one of numeric sound identifiers 33, 34 or 35 using a modulo-three draw, with pitch `1.0`. The ordinary alternative selects identifier 32. These observations support the wet-ground interpretation without claiming that material 3 is natively swimmable. [N4, N5]

## Surface rendering distinction

The mesh producer at RVA `0x09D910` routes material-2 faces to a dedicated index collection, distinct from the ordinary solid-face collection. Its material-2 input tint interpolates between RGBA `(0.1,0.2,1,1)` and `(0.3,0.4,1,1)` using the normalized blue channel. [N2]

Material 3 ordinarily follows the solid-face collection. When its upper neighbor is material 0, the mesh producer additionally emits an upward-facing quad at the cell's top into the same dedicated index collection used by material 2. That extra quad uses RGBA `(0,0,1,1)` as its shading input. The ordinary solid top is also emitted. The extra face's gate is material 3 below and material 0 above, after masking the material bytes; it is not gated by bit `0x80` and does not change the stored material. [N2]

This supplies a native explanation for a shallow surface that looks like water while supporting walking: a material-3 solid can carry a water-stream top face without containing material-2 volume. This is an evidence-based explanation of that material path, not proof that every owner-observed shallow wet patch has that material. No owner capture or runtime probe was examined here.

## Limits and extension boundary

No native material named “shallow water” was established. The verified distinction is material-2 liquid versus material-3 wet solid, with a separate visual top face. The exact native shader, its wave/displacement behavior, water depth rendering, and the mapping of a particular observed location remain unverified in this contract. The complete meaning of all upper bits and all material identifiers also remains open.

Making all Alpha water-like shallow surfaces swimmable, reusing a Steam surface shader, and placing reeds along banks are user-requested extension requirements. They must not be presented as established native Alpha behavior on the strength of this contract. No extension design or replacement-engine mapping is specified here.

## Original evidence references

- **N1:** `decompiled/client/functions/00434c10.c`, RVA `0x034C10`, material mask; `decompiled/client/functions/0043b480.c`, RVA `0x03B480`, solid predicate.
- **N2:** `decompiled/client/functions/0049d910.c`, RVA `0x09D910`, especially lines 876–1007 and 1589–1674. Original instruction confirmation in `decompiled/client/assembly.txt`: RVA `0x09E9C8` material-2 gate, `0x09EB34` dedicated collection, `0x0A01F9` material-3 top gate and `0x0A0236` dedicated collection for that top face.
- **N3:** `decompiled/client/retry-default-180/functions/005e4850.c`, RVA `0x1E4850`, especially lines 1369–1409 and 1833–1951. `decompiled/client/assembly-functions/005e4850.asm`: RVAs `0x1E69A5`–`0x1E6B02` lake path; `0x1E7E85` onward river path; `0x1E81BA` liquid byte 2 and `0x1E82A2` wet-solid byte 3. Floating constants checked directly in `binary/Cube.exe` at RVAs `0x345D98`, `0x345DA0`, `0x345DC0`, `0x345E10`, `0x345E60`, `0x2FCE3C`, `0x2FCD7C`, `0x2FCE24`, `0x31DD48`.
- **N4:** `decompiled/client/assembly-functions/0060c510.asm`, routine RVA `0x20C510`: obstruction at `0x21B660`–`0x21B6BC`; box material/state tests at `0x21CEE6`–`0x21D0C0`; class-6 exclusion at `0x21CF87`; ground and wet-foot tests at `0x21D1DF`–`0x21D31C`; combined wet/class-5 owner path at `0x21D322`–`0x21D385`. The routine's decompilation timed out; these claims rely on the original assembly.
- **N5:** `decompiled/client/assembly-functions/0060c510.asm`, RVAs `0x21EE01`–`0x21EED8`, material-3 footstep selection. Numeric sound identifiers are preserved without asserting textual native names.
- **N6:** `decompiled/client/functions/005c56e0.c`, RVA `0x1C56E0`, final material-12 selection and `0x20` output bit; `decompiled/client/retry-default-180/functions/005e4850.c`, RVA `0x1E4850`, lines 917–942, material-6 solid fill. Color routine: `decompiled/client/functions/005f9620.c`, RVA `0x1F9620`; only its call identity and color-output use are relied on here.
- **N7:** `decompiled/client/functions/006fae90.c`, RVA `0x2FAE90`, default used by lake path; `decompiled/client/functions/006f9780.c`, RVA `0x2F9780`, reader water default; `decompiled/client/functions/006f99c0.c`, RVA `0x2F99C0`, writer water default.
- **N8:** `decompiled/client/functions/0042f7e0.c`, RVA `0x02F7E0`, integer-coordinate terrain reader. The water default is identified independently by N7.
- **N9:** `decompiled/client/functions/0044e7a0.c`, RVA `0x04E7A0`, world writer; `decompiled/client/functions/0044e700.c`, RVA `0x04E700`, column writer and four-byte stride.
- **N10:** `decompiled/client/functions/0042f040.c`, RVA `0x02F040`, three-coordinate conversion; `decompiled/client/functions/0042f100.c`, RVA `0x02F100`, single-coordinate conversion. Fixed-coordinate reader at `decompiled/client/functions/0042f860.c`, RVA `0x02F860`, likewise applies the negative-coordinate adjustment.

The author read this entire final document back after writing it. FINAL / STABLE / HELD: author writes are stopped.
