# Alpha water and wet-ground material contract

Status: FINAL / STABLE / HELD. Source-side native evidence contract, dated 2026-11-07 and re-certified 2026-11-22. This document describes the bounded material, terrain-production, contact and surface-mesh facts below. It does not claim native runtime acceptance and does not prescribe changes to any replacement implementation.

## Evidence boundary and image

Original evidence paths in this document are relative to the `disassembled alpha cube world` evidence root. The supported image is `binary/Cube.exe`: 3,885,568 bytes, SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`, PE machine `0x014C`, PE32 optional-header magic `0x010B`, preferred image base `0x00400000`, image size `0x003BC000`, COFF timestamp `0x51EA955E`. RVAs below are relative to that preferred base. These are client-image facts; server equivalence is not established here.

Every positive material, contact and mesh statement below rests on original Alpha evidence: per-routine decompilation output, original assembly listings and the original image bytes. Floating constants were read directly from the image. Reconstruction material was used only as a search lead in the original authoring pass and is not relied on for any statement. Descriptive terms such as "wet ground", "lake path" or "river pass" are this document's own labels, not native identifiers.

## Material representation and classification

One stored voxel occupies four bytes. Bytes `+0`, `+1` and `+2` carry color channels; byte `+3` carries a five-bit material identifier in its low bits and three upper bits. The native material accessor (RVA `0x034C10`) yields only the low five bits of byte `+3` (mask `0x1F`). The native solidity predicate (RVA `0x03B480`) reports false when those low five bits are 0 or 2 and true for every other value. Upper bits therefore never turn material 3 into liquid or remove its solidity. [N1]

| Low-five-bit value | Evidence-supported meaning in this contract | Native solid predicate | Liquid contact scan |
| --- | --- | --- | --- |
| 0 | Empty/open material | False | Does not set the liquid state |
| 2 | Water volume | False | Sets the liquid state when inside the tested creature box, subject to the creature-class gate below |
| 3 | Wet solid ground; the material that can carry the water-style top face described below | True | Does not set the liquid state |
| 1, 6, 9, 10 and 12 | Examples of other solid materials produced or returned by native terrain routines | True | Do not set the liquid state |

"Wet ground" is a behavioral description: material 3 is chosen explicitly by the terrain producer, can receive an extra water-style top face, participates in wet foot contact and selects a distinct footstep sound family. It is not inferred from its color or from a generic "underwater" flag. The evidence does not establish official native textual names for these numeric materials. [N1–N5]

The natural-surface selector (RVA `0x1C56E0`) can finish with material 9, 10 or 12 and always sets upper bit `0x20` on its output byte. The main terrain producer writes material 6 for its solid fill. The lake and river-bank paths described below construct material 3 instead. Color alone cannot distinguish these cases: the material-3 paths take their color from the same native terrain-color routine (RVA `0x1F9620`) used for other solid terrain. This contract does not assign a general meaning to bit `0x20`. [N3, N6]

## Water volume versus bottom and bank production

The native terrain producer at RVA `0x1E4850` contains two independently identifiable wet-terrain paths. They are described here by their outputs and numeric gates. [N3]

Lake path. The current region record's 32-bit kind at offset `+0x18` must equal 4. The record's horizontal shape routine (RVA `0x1F8E10`, evaluated at the column's x and y converted to 16.16 fixed point) must return a value strictly below `0.25`. The starting height is the record's single-precision value at offset `+0x14` minus `25.0`, converted to an integer by truncation toward zero; the path does nothing if that height is below the column base. Processing then descends one cell at a time down to the column base, inclusive. Each cell is first read through the integer-coordinate reader described below. If the read material is 0 or 2, the cell is written with the default water record: color bytes `(255,255,255)` and packed byte `0x82`, that is material 2 plus upper bit `0x80`. Any other material is replaced by material 3 with native terrain color. All writes go through the world writer. Lake bottom/solid ground and open water volume are therefore explicitly different producer results. [N3, N7, N9]

River pass. For each column, a distance-field value is multiplied by `50.0` and subtracted from `1.0`; a result below `0.0` skips the column. A second field value above the single-precision constant with bit pattern `0x3F733333` (approximately `0.95`) also skips it. A terrain height is then taken, negative values are clamped to zero, and the height is truncated to an integer and rounded down to a multiple of five. That multiple of five is the terraced water level. [N3]

The pass derives a profile value from the fractional position of the height within its five-block interval: the height minus the terraced level, divided by `5.0`. Below `0.5` the profile is that fraction times `2.0`. At or above `0.5` the profile is `1.0` minus four times the excess over `0.5`; if that result is negative it is replaced by the square of (result plus one) minus one. The liquid interval runs from the integer truncation of (level minus five times the profile plus `2.0`) up to the terraced level, inclusive; for small or negative profile values that interval is empty. Each liquid write is skipped when the cell read back has upper bit `0x40` set; otherwise it writes packed byte `2` (material 2 with no upper bits) with red and green zero and blue equal to `255` times (one minus the profile), truncated. [N3]

The bank candidate is the terraced level, raised to the integer truncation of (level minus five times the profile) when the profile is negative. If the candidate is at or above the terraced level, the cell read there is not material 2 and its bit `0x40` is clear, the producer writes material 3 with native terrain color. Only on that successful bank write does it take one draw from the C-runtime pseudo-random function imported by the image; a remainder of zero modulo 200 appends the location to a list used by a later stage. The pass then writes the empty default (packed byte `0`) to cells from one above the candidate up to, but excluding, a noise-dependent ceiling, again skipping cells with bit `0x40` set. The protected-cell check and the material-2 exclusion matter: "near the river" does not mean every surface cell becomes liquid. [N3]

These facts establish producer distinctions and constants. They do not establish the river distance field, the second field, the height source, the noise ceiling, regional shaping, color functions or full generation order. [N3]

## Upper bits and implicit water

Bit `0x40` has a verified protective role in this scope. The integer-coordinate reader (RVA `0x02F7E0`) behaves as follows. A missing column, or a height below the column's stored base, yields the solid default: color `(200,200,200)`, packed byte `0x01`. A height at or above the stored top yields the water default (packed byte `0x82`) when the height is at most 0 and the empty default (color `(255,255,255)`, packed byte `0x00`) when it is above 0. Inside the stored range, a material-0 cell at height at most 0 is reported as the water default unless its bit `0x40` is set. A consumer of native reads can therefore see water where no material-2 cell is stored. [N7, N8]

The world writer (RVA `0x04E7A0`) acts only when the target column exists. It replaces the supplied record with its water default for a write at height at most 0 when the supplied packed byte has no bits in mask `0x5F`. The column writer (RVA `0x04E700`) keeps an existing `0x80` bit on an existing cell: a replacement whose low five bits are 0 is refused, while any other replacement stores the new four-byte record and then sets `0x80` again. That bit is persistent state for a stored cell, not a liquid classification. A material-3 cell that retains `0x80` is still material 3 for the native material, solidity and contact tests. [N9]

The three inspected water defaults, used by the reader, the world writer and the lake path, are each initialized to color `(255,255,255)` and packed byte `0x82`. Not every explicit material-2 write carries `0x80`: the river pass supplies packed byte `2`. [N7, N9]

## Collision and contact gates

The movement routine at RVA `0x20C510` uses the native solidity predicate when testing terrain obstruction. In that test, when bit `0x100` of the 16-bit creature field at offset `+0x7E` is set, any cell that is not material 2 also obstructs, including empty cells. That exception does not make material 3 non-solid. [N1, N4]

During state recomputation the creature's 32-bit flags word at offset `+0x5C` is cleared to zero and rebuilt. The contact region is the inclusive integer-cell box from the position minus half the creature dimensions to the position plus half the dimensions. The conversion divides each 64-bit fixed-point coordinate by `65,536` with truncation toward zero and subtracts one more for a negative input, so exact negative integers also move down one cell. [N4, N10]

The first box scan sets flag `0x10` as soon as it meets a cell whose material is not 2, and stops. A second scan is skipped entirely when the creature-class byte at offset `+0x60` equals 6. Otherwise it sets flag `0x02` for every material-2 cell in the box, sets a local wet-contact result, and keeps the largest water-cell top (cell height plus one), starting from the box's lower height cell, for the following buoyancy step. Material 3 never satisfies either material-2 test. [N4]

The ground check that follows runs only when liquid flag `0x02` is clear. It samples the box's horizontal cell range at a single height `0.2` below the feet (the position minus half the creature height), converted with the same negative-coordinate rule, and sets ground flag `0x01` when the solidity predicate succeeds. In the same pass, a cell whose material equals 3 sets the local wet-contact result. The scan stops after the row in which ground is found, so the material-3 test covers only the cells visited up to that point. Native wet contact therefore merges two cases that the flags keep separate: water volume and feet on wet solid ground. Flag `0x01` is also set by other later ground paths in the same routine that are outside this contract. [N4]

One verified use of the merged wet result: when it is set and the creature-class byte equals 5, the routine looks up another entity through the identifier stored at creature offset `+0x198`. If one is found and its single-precision value at offset `+0x1198` is below `1.0`, that value increases by the routine's time step times `0.05` and is capped at `1.0`. The time step is the routine's first integer argument multiplied by `0.001`, consistent with milliseconds converted to seconds. [N4]

The walking sound path samples the material half a block below the feet. For material 3 it takes one C-runtime random draw and selects numeric sound identifier 33, 34 or 35 from that draw modulo three, with its accompanying scale parameter set to `1.0`. For any other material it selects identifier 32 and takes one draw to set that parameter to `1.0` plus `0.2` times the draw divided by `32767.0`. These observations support the wet-ground interpretation without showing that material 3 is natively swimmable. [N5]

## Surface rendering distinction

The mesh producer at RVA `0x09D910` sends material-2 faces to a dedicated face collection, separate from the collection used for ordinary solid faces. For material 2 its color input interpolates between RGBA `(0.1,0.2,1,1)` and `(0.3,0.4,1,1)` by the cell's blue byte divided by `255.0`. A separate color blend applies to cells below the column base; that blend is not re-derived here. [N2]

Material 3 ordinarily uses the solid-face collection. When the cell above, read through the integer-coordinate reader, has low five bits equal to 0, the mesh producer also emits an upward-facing quad at the cell's top into the same dedicated collection used by material 2. That extra quad uses RGBA `(0,0,1,1)` as its color input and an upward normal. The ordinary solid top face is still emitted. The extra face is gated only on the masked material values (material 3 here, material 0 above); it does not depend on bit `0x80` and does not change the stored material. Because the reader reports water for an empty cell at height at most 0, a material-3 cell whose top is at height 0 or below does not receive the extra face when the cell above is empty and unprotected. [N2, N8]

This gives a native explanation for a shallow surface that looks like water yet supports walking: a material-3 solid can carry a water-style top face without any material-2 volume. It explains that material path; it does not prove that every observed shallow wet patch has that material.

## Limits and extension boundary

No native material named "shallow water" was established. The verified distinction is material-2 liquid versus material-3 wet solid, with an extra water-style top face. The native shader that draws the dedicated water collection, its wave or displacement behavior, depth rendering and the mapping of any particular observed location remain unverified. The full meaning of all upper bits and all material identifiers also remains open.

Separately from the native behavior above, the owner has requested extensions: all Alpha water-like shallow surfaces swimmable, reuse of a Steam surface shader, and reeds along banks. These are owner requests, not native Alpha behavior, and nothing in this contract supports them as native claims. No extension design is specified here.

## Unresolved

- The water shader: the image embeds compiled shader bytecode (strings from the HLSL compiler appear at several image offsets), but which shader draws the dedicated water collection, and its inputs and constants, were not identified.
- The consumer of the 1-in-200 bank location list and what it places there. The earlier wording that it records a vegetation location is not confirmed and is withdrawn.
- The horizontal sample coordinates used by the footstep material read, and the textual names or meanings of sound identifiers 32 to 35 and of the scale parameter.
- The identity of the entity found through creature offset `+0x198` and the meaning of its value at `+0x1198`.
- The origin of the half-dimension values used for the contact box, and the remainder of the buoyancy calculation.
- The river distance field, second field, height source, noise ceiling and generation order; the record shape routine's full behavior per record kind.
- The meaning of upper bits `0x20` and `0x80` beyond the stated reader and writer effects, and the meaning of material identifiers other than 0, 2 and 3.

## Original evidence references

- **N1:** `decompiled/client/functions/00434c10.c`, RVA `0x034C10`, material mask; `decompiled/client/functions/0043b480.c`, RVA `0x03B480`, solidity predicate.
- **N2:** `decompiled/client/functions/0049d910.c`, RVA `0x09D910`, especially lines 876–1007 and 1589–1674. Instruction confirmation in `decompiled/client/assembly.txt`: RVA `0x09E9C8` material-2 gate, `0x09EB34` dedicated collection, `0x0A01F9` material-3 top gate and `0x0A0236` dedicated collection for that top face. Constant `255.0` at RVA `0x2FCE64`.
- **N3:** `decompiled/client/retry-default-180/functions/005e4850.c`, RVA `0x1E4850`, especially lines 1369–1409 and 1833–1951. `decompiled/client/assembly-functions/005e4850.asm`: RVAs `0x1E69A5`–`0x1E6B02` lake path; `0x1E7E85` onward river pass; `0x1E81BA` liquid byte 2; `0x1E82A2` wet-solid byte 3; `0x1E82B4` random draw through the imported C-runtime random function (import slot RVA `0x2FC468`). Floating constants read directly from `binary/Cube.exe` at RVAs `0x345D84` (0.0), `0x345D98` (0.25), `0x345DA0` (0.5), `0x345DC0` (1.0), `0x345E10` (2.0), `0x345E60` (4.0), `0x2FCE3C` (25.0), `0x2FCD7C` (50.0), `0x2FCE24` (5.0) and `0x31DD48` (bit pattern `0x3F733333`). Empty default used by the pass: initializer at RVA `0x2FAE30`.
- **N4:** `decompiled/client/assembly-functions/0060c510.asm`, routine RVA `0x20C510`: obstruction at `0x21B660`–`0x21B6BC`; flags cleared at `0x21CCEB`; box material and state scans at `0x21CEE6`–`0x21D0C0`; class-6 exclusion at `0x21CF87`; ground and wet-foot tests at `0x21D1DF`–`0x21D31C`; merged wet and class-5 path at `0x21D322`–`0x21D385`; time-step scaling at `0x20C83D`–`0x20C862` with `0.001` at RVA `0x2FD870`; rate `0.05` at RVA `0x2FCDB4`. The routine's decompilation did not complete; these statements rely on the original assembly.
- **N5:** `decompiled/client/assembly-functions/0060c510.asm`, RVAs `0x21EE01`–`0x21EED8`, footstep material read and selection; constants `0.2` at RVA `0x2FCDBC` and `32767.0` at RVA `0x2FD338`. Numeric sound identifiers are given without native textual names.
- **N6:** `decompiled/client/functions/005c56e0.c`, RVA `0x1C56E0`, material 9, 10 and 12 outputs and the `0x20` output bit; `decompiled/client/retry-default-180/functions/005e4850.c`, RVA `0x1E4850`, lines 917–942, material-6 solid fill. Color routine: `decompiled/client/functions/005f9620.c`, RVA `0x1F9620`; only its identity and color-output use are relied on.
- **N7:** `decompiled/client/functions/006fae90.c`, RVA `0x2FAE90`, water default used by the lake path; `decompiled/client/functions/006f9780.c`, RVA `0x2F9780`, reader water default; `decompiled/client/functions/006f99c0.c`, RVA `0x2F99C0`, world-writer water default; `decompiled/client/functions/006f9720.c`, RVA `0x2F9720`, reader empty default; `decompiled/client/functions/006f9750.c`, RVA `0x2F9750`, reader solid default.
- **N8:** `decompiled/client/functions/0042f7e0.c`, RVA `0x02F7E0`, integer-coordinate terrain reader.
- **N9:** `decompiled/client/functions/0044e7a0.c`, RVA `0x04E7A0`, world writer; `decompiled/client/functions/0044e700.c`, RVA `0x04E700`, column writer and four-byte stride.
- **N10:** `decompiled/client/functions/0042f040.c`, RVA `0x02F040`, three-coordinate conversion; `decompiled/client/functions/0042f100.c`, RVA `0x02F100`, single-coordinate conversion. The fixed-coordinate reader entry at `decompiled/client/functions/0042f860.c`, RVA `0x02F860`, applies an equivalent negative-coordinate adjustment before calling N8.

## Re-certification (2026-11-22)

A fresh source-only author, working in a separate context, re-checked every native statement in this document against the original client image, its per-routine decompilation and its original assembly; the image hash, header fields, cited RVAs and floating constants were confirmed directly. Corrections: the claimed vegetation purpose of the 1-in-200 bank location list was withdrawn as unconfirmed, the "pitch 1.0" footstep wording was narrowed to an unnamed scale parameter and the non-wet footstep's randomized parameter was added, and the time-step unit was restated as an integer argument scaled by 0.001 rather than asserted seconds. Added precision covers the solid and empty reader defaults, the obstruction exception's field and bit, the flags-word clear, the river profile shape, the post-bank empty fill, the selector's materials 9 and 10, the ground-scan early stop and the top-face height-zero case. Shader identity, the bank-list consumer, buoyancy details, field functions and the remaining upper-bit and material meanings stay open as listed under Unresolved. FINAL / STABLE / HELD: author writes are stopped.
