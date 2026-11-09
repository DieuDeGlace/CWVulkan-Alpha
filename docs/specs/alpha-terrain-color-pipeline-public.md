# Alpha ordinary opaque terrain color

This is a bounded source-only contract for the original client's compact opaque cube-face draw path. It establishes the packed vertex-color conversion, the selected vertex and pixel programs, their lighting and fog relations, and several consequential constant-update precedences. It does not establish the complete upstream terrain mesher or the final render-target transfer function. Those limits are explicit below.

No replacement implementation, earlier specification, SDK source, extractor, test, probe, runtime capture, log, or implementation history was read. Shader programs were decoded in memory from original executable bytes; this document contains independently worded behavioral prose, not shader source or instructions.

## Image and source anchors

All evidence paths are relative to `disassembled alpha cube world`. The supported original is `binary/Cube.exe`: 3,885,568 bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`, PE machine `0x014C`, preferred base `0x00400000`, timestamp `0x51EA955E`, image size `0x003BC000`. Routine addresses below are RVAs; addresses explicitly called virtual addresses include the preferred base.

| Original program | Executable file offset | RVA | Byte length | SHA-256 of this byte span |
| --- | --- | --- | ---: | --- |
| Shared compact cube-face vertex program | `0x002FC928` | `0x002FD928` | 4,648 | `c42acf58dbfdb7c5545c7f3164fad864815ef98d56b42dc1a18299ad528fed8e` |
| Ordinary opaque pixel program | `0x002FDB50` | `0x002FEB50` | 1,100 | `773c20c6c523a4b59dd0d9cc5897c65516e9bc39ef6211e9af3678c35ae39d14` |

Both are shader-model-3 programs. Native initialization at RVA `0x047E10` creates these programs from the indicated original bytes. Its ordinary bind at RVA `0x047D10` selects the declaration, this vertex program, and this opaque pixel program. Different native binds select other pixel programs for other passes; their arithmetic is not substituted for the opaque program here.

Primary routine evidence: `decompiled/client/functions/00447e10.c`, `00447d10.c`, and the matching spans of `decompiled/client/assembly.txt`. The main frame renderer is at RVA `0x0AC260`; its large decompilation is incomplete, so frame ordering is anchored to original assembly rather than an invented high-level reconstruction.

## Packed inputs and normalization

The compact draw uses an eight-byte vertex stride. Its declaration supplies four unsigned bytes at byte offset zero as the position input: three coordinates and a face selector. This input is not normalized to a unit interval. At byte offset four it supplies a packed Direct3D color as the color input. Its red, green, blue, and alpha channels each have eight bits and arrive at the shader normalized by `255`. Thus a packed color-channel value of `0` becomes `0`, `255` becomes `1`, and an intermediate value is its byte value divided by `255`. This conversion is a normalized format conversion; it is not a gamma or sRGB decoding operation.

The position's fourth byte chooses a cube-face normal. Selectors `0`, `1`, `2`, `3`, `4`, and `5` correspond respectively to positive X, negative X, positive Y, negative Y, positive Z, and negative Z. In this program selectors outside `0..4` share the negative-Z result. The fourth position byte is not a homogeneous position coordinate. The vertex program uses a homogeneous coordinate of one for the position transform.

The face normal is transformed by the world matrix's directional portion and normalized. This is the world-direction transform present in the original shader, not an independently assumed inverse-transpose correction. The CPU light-direction setter separately normalizes its supplied three-component direction before upload.

The packed color's alpha participates in vertex lighting. It is also passed onward as a color component, but the ordinary opaque pixel program takes its output alpha from a separate scalar constant. Consequently the packed alpha is not, by itself, the opaque terrain's output transparency.

Anchors: declaration construction at RVA `0x047E10`, especially virtual addresses `0x00447E20..0x00447E70`; eight-byte stream binding and indexed triangle draw at virtual addresses `0x004B1D45..0x004B1D9A`; vertex-program byte span identified above; direction upload in `decompiled/client/functions/00448170.c`, RVA `0x048170`.

The upstream mapping from a stored voxel's RGB bytes to these packed vertex bytes has not been fully audited in this assignment. In particular, this contract does not establish that every mesher path copies voxel RGB unchanged, nor does it supply a complete rule for vertex alpha, corner darkening, neighbor-dependent color changes, or mesher quantization. The normalization statement applies exactly to the packed bytes submitted on this draw path.

## Vertex lighting and color arithmetic

Lighting is evaluated at vertices. The pixel program later receives the interpolated result; it does not reevaluate the directional and point lighting per pixel.

The vertex program starts with a three-component base illumination comprising ambient color plus two directional contributions. The front contribution uses the nonnegative part of the dot product between the normalized world normal and the supplied light direction. The back contribution uses the nonnegative part of the dot product against the opposite light direction. Each contribution multiplies its respective RGB light color. Negative directional dot products contribute zero; there is no separate upper clamp of these directional contributions at this stage.

That combined ambient-and-directional illumination is multiplied by both the surface-tint alpha and normalized vertex alpha. Point lights are added afterward. The distinction matters: those two alpha factors attenuate the ambient and directional portion before point-light addition, rather than multiplying the completed illumination sum.

The compiled program consumes ten point-light slots, in ascending slot order `0..9`. A slot is enabled only when its fourth position component, used as radius, is strictly greater than zero. The shader interface contains larger array declarations, but this compiled arithmetic and the audited upload helper use ten slots. Sixteen declared or prepared elements do not establish sixteen contributing lights.

For a well-defined enabled point light, radial attenuation begins with squared distance divided by squared radius, limited above to one. Subtracting that result from one and squaring gives the radial factor. At and beyond the radius the radial factor is zero. The angular factor uses the dot product between the normalized world normal and the normalized direction from the light toward the vertex, multiplies it by negative four, and limits it to the interval from `0.3` through `1`. The resulting point-light RGB contribution is its light color multiplied by both factors. Thus this angular response retains a `0.3` floor rather than behaving as a zero-floor Lambert term. Zero-distance normalization and exceptional floating-point inputs are outside the finite, well-defined relation stated here.

The first point contribution is combined with the scaled base illumination, followed by the remaining point contributions in slot order. Only after this accumulation is each illumination channel limited to `0..1.2`. The original upper constant has binary32 value approximately `1.20000005`; the angular floor has binary32 value approximately `0.300000012`.

The clamped illumination then multiplies the normalized vertex RGB and surface-tint RGB, channel by channel. A second, additive contribution uses the dark-fill RGB multiplied by one minus the same clamped illumination. The dark-fill weight is not recomputed from the already tinted vertex result. Since illumination can reach `1.2`, that complement can be negative. The shader does not clamp the complement to zero.

Finally, a whitening scalar blends this vertex RGB result toward white. At zero it preserves the result; at one it produces white. The audited vertex arithmetic does not explicitly saturate the completed RGB after the dark-fill and whitening steps. Its earlier `0..1.2` limit is an illumination limit, not a promise that every subsequent color stays within `0..1`.

Anchors: original vertex-program bytes at RVA `0x2FD928`; constant-slot initialization in `00447e10.c`; ambient/directional upload in `00448170.c`; point upload in `00448f10.c`, RVA `0x048F10`; surface tint in `00448280.c`, RVA `0x048280`; dark fill in `00448070.c`, RVA `0x048070`; whitening in `00449090.c`, RVA `0x049090`.

## Day factors and constant precedence

The main frame renderer computes a material daylight envelope from the signed 32-bit time value at controller `+0x800440`. It converts the time to binary32, doubles it, divides by binary32 `86,400,000`, and subtracts one. It raises that intermediate to the fourth power, subtracts the rounded result from one, then raises the result to the fifth power. Power operations use binary64 inputs and results with binary32 rounding between the stages shown by the native call sequence. This produces zero at the two day endpoints and one at the midpoint for the ordinary in-range inputs. No clamp or wrap for arbitrary out-of-range time is supplied by this computation itself.

For the ordinary dry case the prepared surface tint has RGB `(1, 1, 1)` and this daylight envelope as alpha. The nearby native submerged-material condition changes RGB to `(0.4, 0.5, 1)` while retaining the daylight alpha. That native condition is distinct from an assumed global color transfer. At the compact opaque batch entry the renderer uploads the prepared surface tint again, so intervening object tint changes do not establish this batch's tint.

The environment color producer uses another envelope. Its source is the signed 32-bit value at world `+0x80015C`. After the same binary32 time scaling to a value centered at zero, it takes the absolute value, raises that to the third power, subtracts the rounded result from one, and raises the result to the fourth power. It is therefore a different envelope from the material's fourth-power/fifth-power sequence. This contract does not prove that the two source fields are always equal.

The environment routine produces the sky, fog, ambient, and directional colors used by the frame. It also consults spatial/environment functions and the voxel-type condition at the supplied position. Its palette has branch thresholds and environmental blends; treating it as one fixed daylight RGB multiplier is unsupported. The full palette and its spatial dependencies are not independently closed here.

A particularly consequential later update is established. Before the ordinary compact opaque batch, the frame adds the current two sky-color vectors and halves their sum. It uploads the resulting RGB as dark fill. This later upload takes precedence over an earlier dark-fill upload of zero. The opaque shader bind does not reset it. The tint, illumination, and dark-fill arithmetic above must therefore retain that later sky-derived fill rather than silently assuming a zero fill for ordinary terrain.

At the opaque batch entry the frame restores the ordinary shader pair, uploads its ambient/front/back colors and normalized direction, and updates the fog-scale value. For each nonempty region batch it uploads surface tint and the point-light arrays and explicitly sets the separate output-alpha constant to one. These are separate state updates. Binding a shader program does not itself replace all of its constants, matrices, render states, or previous-pass values.

Anchors: material daylight sequence at virtual addresses `0x004AC3E1..0x004AC44A` in `decompiled/client/assembly.txt`; binary64 exponents at virtual addresses `0x00702B50` and `0x00702B58` are `4` and `5`. Material preparation and native alternate tint are at `0x004AD684..0x004AD750`. Environment production is `decompiled/client/functions/0059d640.c`, RVA `0x19D640`, with its different exponent sequence checked at `0x0059D65B..0x0059D708`; its first exponent at `0x006FCB98` is `3`, its second at `0x00702B50` is `4`.

The later dark-fill average is at virtual addresses `0x004B1433..0x004B149C`, using the native vector addition and scalar multiplication in `00468f50.c` and `004a8f40.c`. Ordinary opaque setup is at `0x004B1A6B..0x004B1AB5`; per-region tint, point-light, and alpha updates are at `0x004B1BE7..0x004B1C1E`. Output-alpha upload is `00447fb0.c`, RVA `0x047FB0`.

## Fog inputs and opaque pixel result

The vertex program supplies two screen-derived coordinates: projected clip X divided by clip W, and projected clip Y divided by clip W with a vertical fog-gradient translation added. It also supplies full three-dimensional squared camera distance and horizontal squared camera distance, each multiplied by the square of a fog-scale constant. The full squared distance occupies the third fog-coordinate component; horizontal squared distance occupies the fourth. Squaring occurs before interpolation of these fog coordinates.

The fog-scale setter updates the constant only when its supplied distance is strictly positive. In that case it uploads the reciprocal of that distance. A zero, negative, or unordered input does not upload a replacement through this setter and therefore does not imply that the previous fog scale becomes zero. The vertical translation setter subtracts `80` from its supplied value and divides by `-30`. The audited frame selects either its supplied angle value or the fixed value `90` for that setter under the nearby control flag; this is separate from the distance scale.

The opaque pixel program computes a sky-gradient target using the interpolated projected vertical coordinate. It limits `0.6` times that coordinate to `0..1`, then takes three times the square of that limited value minus twice its cube. This cubic transition is zero at zero, one at one, and has zero endpoint slopes. It mixes the two sky RGB colors. A second application of the same cubic transition, using twice the vertical coordinate plus one limited to `0..1`, mixes between a base color made from `0.2` times the first sky color plus `0.8` times fog color, and the first sky mixture. These two transitions establish the height-dependent far target.

The near fog weight uses the full-distance component. It first scales that component by `1.5`, subtracts `0.025`, and limits the result to `0..1`. Its weight is `0.75` times one minus the square of the complement of that limited result. The weight is zero through the lower threshold and reaches `0.75` at and above the upper threshold. This mixes interpolated lit RGB toward fog RGB. Near fog alone therefore does not necessarily replace the full terrain color, even at its upper limit.

The far fog weight uses the horizontal-distance component. It subtracts `0.6`, multiplies by `2.5`, and limits the result to `0..1`. It applies the same cubic smooth transition, then squares that transition. This weight mixes the near-fogged result toward the height-dependent far target. Far fog begins when the horizontal squared-distance component exceeds `0.6` and reaches complete replacement at component `1`. Equality at the lower endpoint has zero far weight; equality at the upper endpoint has full far weight.

The two fog weights are not interchangeable: one uses full distance and a maximum weight of `0.75`, while the other uses horizontal distance and can completely replace the near-fogged result. Neither is a simple linear camera-distance fog.

The pixel program also has a separately controlled periodic modulation. A nonpositive control preserves the fogged RGB. A positive control selects a cosine-derived multiplier ranging from approximately `0.7` to `1.3`; its existence does not establish that every ordinary terrain draw enables it. The frame explicitly uploads zero at virtual address `0x004AD9A9`, while the ordinary bind does not reset that control. Its complete possible mutation through all intervening draw callees has not been audited here; disabling it universally is therefore not a held global invariant.

The opaque pixel program writes the separate alpha constant. With the per-region native upload of one, its output alpha is one. It does not derive output alpha from the packed vertex alpha. The audited opaque RGB program has no explicit final RGB saturation or gamma power transfer after its fog mixture and optional modulation.

Anchors: original vertex and opaque pixel byte spans listed above; distance setter `decompiled/client/functions/00448100.c`, RVA `0x048100`; vertical translation setter `00448090.c`, RVA `0x048090`; frame inputs at virtual addresses `0x004AD9D1..0x004ADA3D` and `0x004B1AA1..0x004B1AB5`; fog/sky color upload `00449040.c`, RVA `0x049040`; periodic-control upload `00448fe0.c`, RVA `0x048FE0`.

## Render-target encoding and remaining state limits

The audited declaration performs numeric normalization of packed color. The audited vertex and opaque pixel programs do not explicitly apply sRGB decoding, an sRGB encoding curve, or a conventional output-gamma exponent. These shader facts do not determine the entire displayed transfer function.

The effective Direct3D render-target format, sRGB-write enable state at the exact opaque draw, any later full-screen transformation, presentation gamma ramp, and display transfer have not been closed by this audit. The absence of a discovered immediate state assignment is not sufficient evidence that no other state path can alter it. This contract consequently does not assert either a final linear write or a final sRGB write for the original client. It also does not authorize inserting a presumed gamma correction solely because the input originated as RGB bytes.

The native main renderer visibly updates render states across passes, while the compact shader bind updates only its declaration and shader selections. For example, its frame source/destination blend-factor writes occur before the compact batch, and the batch explicitly supplies output alpha one; neither fact proves every inherited blend, target, or color-write state at that draw. Such effective states require tracing all intervening state-setting and state-block paths. API interpolation precision and any API-imposed color-varying clamping are likewise not established by shader arithmetic alone.

The held facts are the identified programs, input format normalization, evaluated lighting relation and clamp placement, separate day envelopes, later sky-average dark-fill precedence, fog relations and equality boundaries, and the explicit constant-upload conditions. The full mesher-to-vertex mapping, complete palette dependencies, all inherited GPU states, final gamma/sRGB target behavior, and runtime visual acceptance remain unresolved. No runtime parity claim is made.
