# Distance, draw order and model keys for active terrain decorations

**Source author status — FINAL / STABLE / HELD**

## Scope and evidence

The evidence is the owner's original 32-bit Cube.exe, 3,885,568 bytes, SHA-256 84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF, plus its native disassembly and matching function listings. All addresses are RVAs from preferred base 0x400000. The ordinary model route here uses the decoration list at terrain render-slot offset 0x240. A separate array supplies models for indexed world statics.

## Distance inputs and admission

Client construction sets the horizontal window width at 0x2DC to 40. It initializes the float at client offset 0x1D0 to half that width times 32, ordinarily 640 world units. The model pass reads this field and multiplies it by the single-precision constant 0.2 to obtain its outer distance, ordinarily 128. It multiplies that result by the single-precision constant 0.9 for the inner distance, ordinarily about 115.2. The native order matters at a boundary. The adjacent 0.3 multiplier in the renderer feeds another value. Client RVAs 0x059F39, 0x05AB13–0x05AB36, 0x0AD9D1–0x0ADA12 and 0x0B0740–0x0B076C.

The slot must first have primary mesh groups, the expected horizontal slot keys and an admitted slot bound. The slot pass also checks the horizontal squared distance from its reference at slot offset 0x50 to the three-coordinate client reference at 0x140, using X and Y only. It admits equality with the square of the decoration outer distance plus 32, with the addition performed before squaring. The renderer then walks the active decoration list in its stored order, numbering visits from one. ID 60 branches to a separate timed random-effect route before ordinary model lookup. Negative IDs, IDs beyond the model table, ID 18 and null model pointers skip the ordinary draw. RVAs 0x0AC665–0x0AC737, 0x0B1BD7–0x0B1C23, 0x0B1DDE–0x0B1F21 and 0x0B25AF–0x0B25F0.

For an ordinary model, the source position consists of three signed 64-bit fixed-point coordinates at decoration offsets 8, 0x10 and 0x18. One world unit is 65,536 coordinate increments. The client retains X and Y and raises Z by half the model's Z dimension times the decoration's float scale at 0x20. It computes that rise in single precision, converts it back to fixed point and adds it to the original Z. Model dimensions X, Y and Z are the three integers stored at model offsets 0x44, 0x48 and 0x4C. The plane-test radius is the larger of X and Z times the same scale. The radius takes the full larger dimension. Y, model yaw and any subsequent animated tilt do not enter this bound. RVAs 0x0E7290, 0x0E7650–0x0E767C, 0x024810, 0x07B5F0, 0x02C580–0x02C5A0 and 0x0B25F0–0x0B26B9.

The outer test compares the raised center against the reference at client offsets 0x140, 0x148 and 0x150. The native routine squares each fixed-point coordinate difference, divides each product by 65,536 with integer precision and sums the three terms. It compares the result with the integer conversion of the float outer distance squared times 65,536. Equality passes. The radius does not extend this radial limit. The six view-plane tests then use the raised center with the renderer's fixed X/Y translation at client offsets 0x1D8 and 0x1E0. The Z coordinate stays in the original vertical frame. A negative signed plane evaluation after adding the radius rejects the model. Tangency passes. RVAs 0x03AC20, 0x07F760–0x07F92F and 0x0B2693–0x0B26C1.

For an admitted model the inner test rebuilds the same center, subtracts the same reference, converts each resulting component to float world units and adds its three squared components. It draws the model immediately when that float result is at most the square of the inner distance. A larger result enters the deferred list. The stored sort key is this float squared distance. The outer and inner tests share a geometric center but take different numeric paths. An object just touching a view plane and one just reaching the inner distance can both draw. RVAs 0x02C4A0, 0x024860, 0x0B26C1–0x0B2806.

## Deferred ordering and blending

Each appended 16-byte entry retains the decoration pointer, an empty alternate pointer, the squared distance and the original visit number. The same array later receives eligible entries from the indexed-static pass, which uses the alternate pointer and the separate model table at client offset 0x800724. The renderer sorts the combined array in descending squared distance, then traverses it from beginning to end. A deferred decoration therefore draws in farthest-first order among these statics and other decorations, whereas an immediate decoration draws at its visit. The original visit number goes back to the model renderer. The comparator only inspects the float at entry offset 8. RVAs 0x0B276C–0x0B27B6, 0x0B2C95–0x0B2D17, 0x0B3035–0x0B3092, 0x0B32ED–0x0B3335 and 0x0BA467–0x0BA588.

When the entire deferred array has at most 32 entries, insertion sorting moves an entry past strictly smaller distances and preserves the order of equal finite keys. Longer arrays can partition entries, exchange them and take a heap-sort path. The comparator supplies no model-ID, visit-number or object-kind tie break. Stable arrival order for all equal-distance objects in a larger frame is unestablished. RVAs 0x0ABAC0, 0x0AA490, 0x0A9470–0x0A950E, 0x0AAEB0 and 0x0AA7C0.

The decoration draw helper uploads opacity 1 through the inner boundary. Beyond it, the helper takes the square root of the stored distance key and linearly reduces opacity from 1 at the inner distance to 0 at the outer distance. It uploads that scalar separately from the per-decoration lighting/tint parameter. The ordinary scene enables alpha blending with source alpha and inverse source alpha. Depth testing is enabled before the final terrain-group pass. Immediately before walking the deferred vector, the client explicitly enables depth writes and selects clockwise-face culling. It visits the decoration pointer before the alternate pointer within each entry. The subsequent native indexed draw requires both vertex and index buffers. RVAs 0x0AEDC0–0x0AEE12, 0x0BA15C–0x0BA199, 0x0BA432–0x0BA462, 0x0BA4A4–0x0BA54F, 0x0BE606–0x0BE690, 0x047FB0 and 0x0E6DF0.

## Decoration IDs and model names

The client creates 64 zero-filled pointer slots at client offset 0x800718 and assigns the following model-library entries. The two numeric columns in each half give the decimal decoration ID and hexadecimal library index. Names are the original model loader's basename keys. Read a row by ID independently of its neighbor. A dash marks a slot with no assignment in the stock initialization. ID 60 retains the separate effect behavior described above. The original loader spells the ID 51 basename “torch-read.cub”. RVAs 0x062545–0x062BF9, 0x0120C0, 0x1174B3–0x118574 and 0x0E7290.

| ID | Library index | Model key | ID | Library index | Model key |
| ---: | ---: | --- | ---: | ---: | --- |
| 0 | 0x977 | flowers2.cub | 32 | 0x992 | water-lily02.cub |
| 1 | 0x978 | flowers.cub | 33 | 0x993 | inn-sign.cub |
| 2 | 0x979 | grass.cub | 34 | 0x994 | shop-sign.cub |
| 3 | 0x97A | grass2.cub | 35 | 0x995 | weapon-shop-sign.cub |
| 4 | 0x97B | grass3.cub | 36 | 0x996 | armor-shop-sign.cub |
| 5 | 0x99F | underwater-plant.cub | 37 | 0x997 | identifier-sign.cub |
| 6 | 0x9A1 | coral.cub | 38 | 0x998 | smithy-sign.cub |
| 7 | 0x9A0 | alga.cub | 39 | 0x999 | carpentersshop-sign.cub |
| 8 | 0x97C | lava-flower.cub | 40 | 0x99A | weavingmill-sign.cub |
| 9 | 0x97D | lava-grass.cub | 41 | 0x99B | ivy.cub |
| 10 | 0x97E | thorn-plant.cub | 42 | 0x99C | wall-roses-red.cub |
| 11 | 0x97F | echinacea2.cub | 43 | 0x99D | wall-roses-white.cub |
| 12 | 0x980 | leaf.cub | 44 | 0x9A2 | inca-art1.cub |
| 13 | 0x981 | lantern02.cub | 45 | 0x9A3 | inca-art2.cub |
| 14 | 0x982 | torch.cub | 46 | 0x9A4 | inca-art3.cub |
| 15 | 0x983 | stone.cub | 47 | 0x9A5 | inca-art4.cub |
| 16 | 0x984 | stone2.cub | 48 | 0x9A6 | crest1.cub |
| 17 | 0x99E | christmas-tree.cub | 49 | 0x9A8 | wall-skull.cub |
| 18 | — | — | 50 | 0x9A7 | castle-chain.cub |
| 19 | 0x985 | tendril.cub | 51 | 0x9A9 | torch-read.cub |
| 20 | 0x986 | tulips-colorful.cub | 52 | 0x9AA | torch-green.cub |
| 21 | 0x987 | cornflower.cub | 53 | 0x9AB | torch-blue.cub |
| 22 | 0x988 | reed.cub | 54 | 0x9AC | torch-yellow.cub |
| 23 | 0x989 | pumpkin-leaves.cub | 55 | 0x9AD | liana.cub |
| 24 | 0x98A | pineapple-leaves.cub | 56 | 0x9AE | chandelier.cub |
| 25 | 0x98B | sunflower.cub | 57 | 0x9AF | cobwebs.cub |
| 26 | 0x98C | bean-tendril.cub | 58 | 0x9B0 | cobwebs2.cub |
| 27 | 0x98D | desert-flower01.cub | 59 | — | — |
| 28 | 0x98E | desert-flower02.cub | 60 | — | — |
| 29 | 0x98F | wheat.cub | 61 | — | — |
| 30 | 0x990 | corn.cub | 62 | — | — |
| 31 | 0x991 | water-lily01.cub | 63 | — | — |

## Limits

These observations cover ordinary finite coordinates, valid model dimensions and the original slot-based pass. The client reference and native model dimensions specify the inputs directly. The contract does not establish every camera-mode producer of the reference, a general final-pixel result for intersecting meshes, runtime asset availability, or a stable tie policy for larger deferred arrays. The table records registrations and leaves the world-generation conditions for each ID open.
