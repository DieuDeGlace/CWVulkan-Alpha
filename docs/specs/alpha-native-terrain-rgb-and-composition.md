# Native terrain RGB and composition

**FINAL / STABLE / HELD**

## Supported image and draw

This contract covers the original 32-bit Alpha Cube.exe with SHA-256
84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf.
Addresses are RVAs relative to image base 0x00400000. The scope is
the primary indexed terrain draw at 0x0b1d9a and the application
frame that presents it.

The loader creates the terrain vertex and pixel shaders from embedded
vs_3_0 at 0x2fd928 and ps_3_0 at 0x2feb50. The draw binds that pair
through 0x047d10 at 0x0b1a71. Its stream-zero declaration is
POSITION0 of type UBYTE4 at byte 0 and COLOR0 of type D3DCOLOR at
byte 4. The stream offset is zero and the stride is eight
(0x047e25–0x047f1f, 0x0b1d49–0x0b1d9a).

The color word is AARRGGBB, in B, G, R, A memory order. The declaration
presents those bytes to vertex COLOR0 as normalized RGBA. The byte
at vertex offset +7 is the fourth component, *a*, equal to that
unsigned byte divided by 255. The RGB bytes supply the other three
components in R, G, B order. The native mesh upload copies the
staged bytes to the vertex buffer at 0x0a0b39. The primary shader
operates on these normalized components and has no color-space
decode of its own.

## Inputs to the terrain lighting

The vertex shader takes the three integer position components from
POSITION0 and uses homogeneous position W = 1. The fourth byte is
the axis-normal selector. Selectors 0 through 5 are +X, −X, +Y,
−Y, +Z, −Z. In particular the upward face uses +Z. The shader
takes the three dot products of that normal with the XYZ parts
of the world transform rows at c27–c29 and normalizes the result
to obtain *N* (0x2fdcac, 0x2fde04–0x2fded8).

The front, back and ambient RGB vectors are vertex c34, c35 and
c38. The primary draw uses the fourth, fifth and sixth outputs
from the environment producer at 0x19d640. The draw uploads them
through 0x048170 at 0x0b1a9c. Vertex c33 supplies the direction
*D*. The frame starts from (0, −sin θ, −cos θ), where θ is
2π times the signed millisecond clock divided by 86,400,000.
The native frame converts the clock to binary32 and doubles it
before the binary64 π multiply and division. It rounds the angle
to binary32 before calling sine and cosine in binary64, rounds the
results to binary32, and normalizes the vector for the upload
(0x0ad7a6–0x0ad822, 0x0481ce–0x04825d).

Vertex c32 is materialColor. Its RGB is (1, 1, 1), or (0.4, 0.5, 1)
if the camera-cell lookup returns low-five-bit tag 2. The draw
uses the same material alpha in either case. In the frame the
signed clock is at client +0x800440. Convert the clock to
binary32, then form *u* = (2 × clock) / 86,400,000 − 1 in that order.
Raise *u* to the fourth power in binary64 and round to binary32.
Subtract the result from 1 in binary32. Raise the difference to
the fifth power in binary64 and round to binary32 for materialColor.w.
The routine applies this curve directly to the signed clock
(0x0ac3e1–0x0ac44a, 0x0ad684–0x0ad750, 0x0b1be7–0x0b1bf5).

The chunk's ten local light position/radius vectors go to vertex
c0–c9, and their RGB vectors go to c10–c19
(0x0b1c0b, 0x048f10). They enter the vertex RGB calculation
separately from the front, back and ambient inputs.

## Vertex RGB and saturation

Take the dot product of *N* with *D*. The front weight is its
positive part, and the back weight is the positive part of its
negation. Add the weighted front RGB to ambient RGB, then add the
weighted back RGB. Scale this result by *a* times materialColor.w
(0x2fddf4, 0x2fdee4–0x2fdf44, 0x2fe070).

Accumulate the ten point light contributions in slot order. Each
slot is gated by positive radius. For a positive radius *r*, use
the world-space displacement *V* from light center to the vertex
and its squared length *d²*. The distance factor is
(1 − min(1, *d²* / *r²*)) squared. The facing factor is the
dot product of *N* and the normalized *V*, multiplied by −4
and constrained to [0.3, 1]. The product of these factors
scales the slot's RGB. The first point-light RGB is added to
the already scaled ambient/directional result. Subsequent slots
continue that accumulation. The shader uses reciprocal and
reciprocal-square-root for these operations
(0x2fdf58–0x2fea4c).

Constrain each component of the combined lighting to [0, 1.2].
Multiply that lighting by the normalized vertex RGB and then
by materialColor.rgb, channel by channel. The shader also adds
darknessColor.rgb from c37 times one minus the constrained
lighting, then moves the result toward (1, 1, 1) with weight
white.x from c39. The primary terrain setup supplies zero RGB
for darknessColor at 0x0b1aef and initializes white.x to zero
at 0x0ad73d. Both operations therefore leave the ordinary color
multiplication in effect. The lighting cap occurs before these
color operations (0x2fea60–0x2fead4).

The shader outputs the shaded RGB and forwards *a* in COLOR0.w
at 0x2feb34. The primary pixel shader declares only COLOR0.rgb at
0x2fece0. It reads the interpolated shaded RGB.

## Fog and primary pixel RGB

The vertex shader sends the following varyings through TEXCOORD0.

| Component | Value at the vertex |
| --- | --- |
| X | Clip X divided by clip W |
| Y | Clip Y divided by clip W, plus fogGradientTranslation.x from vertex c31 |
| Z | Squared three-axis distance to cameraPosition.xyz from vertex c36, times fogScale.x squared |
| W | Squared horizontal distance to that camera position, times fogScale.x squared |

The distances use camera and vertex positions in the same coordinate
frame, after the position transform at c27–c29. These ratios and
squared distances are formed at the vertices before interpolation
(0x2fdcc0–0x2fddc8).

The upload at 0x048100 writes the reciprocal of a positive distance
to fogScale.x. The primary draw supplies the client float at
+0x1d4 at 0x0b1ab5. This helper leaves the register as it was for
a nonpositive distance. The gradient upload at 0x048090 writes
(input − 80) / −30 in binary32. The frame uses the client float
at +0x1a4, or 90 when its byte +0x8006e4 is nonzero
(0x0ada17–0x0ada3d).

Pixel c0, c1 and c2 contain the first sky RGB, second sky RGB
and fog RGB from the environment producer. They are uploaded in
that order at 0x0ad9cc and 0x0aed8c through 0x049040. For the
following arithmetic let *x*, *y*, *d* and *h* be the interpolated
TEXCOORD0 components. Denote by *S* the smoothstep curve that
first constrains its input to [0, 1], then returns *u²(3 − 2u)*
for the constrained input *u*.

1. Mix first sky toward second sky by *S*(0.6 × *y*). Form the
   other endpoint from 0.2 times first sky plus 0.8 times fog.
   Mix that endpoint toward the sky result by *S*(1 + 2 × *y*).
   This gives the far gradient (0x2fed04–0x2fedfc).
2. Constrain 1.5 × *d* − 0.025 to [0, 1] to obtain *g*. Move
   the interpolated vertex RGB toward fog RGB by the weight
   0.75 × (1 − (1 − *g*)²) (0x2fee74–0x2feebc).
3. Move that nearer result toward the far gradient by
   *S*(2.5 × (*h* − 0.6)) squared
   (0x2fee10–0x2fee64, 0x2feed0).

The pixel shader has an optional RGB modulation for positive
shininess.x at pixel c4. Let *qₓ*, *qᵧ* be the interpolated XY
of TEXCOORD1. The vertex shader supplies twice the normalized
view-space normal in that varying (0x2feae8–0x2feb24). The
modulation is one plus 0.3 times the cosine of a wrapped phase.
For the phase, multiply (10 × *y* + 5 × *x* + *qᵧ* + *qₓ*)
by the shader literal 0.15915494, add 0.5, take the fractional
part, multiply by 6.2831855, and subtract 3.1415927. At
shininess.x ≤ 0 the shader chooses the unmodulated RGB. The
ordinary frame sets shininess.x to zero at 0x0ad9a9
(0x2feee4–0x2fef78).

The pixel shader writes alpha from pixel c3.x. The indexed terrain
draw uploads 1.0 there at 0x0b1c1e. The primary pixel shader samples
no texture and does not apply an extra output RGB clamp or transfer
curve (0x2fece0–0x2fef98).

## Render target and presentation

The world frame clears the current color and depth target and
begins the scene (0x0ac312–0x0ac392). The primary world path
enables RGB alpha blending, with source alpha and inverse source
alpha as its RGB factors (0x0aedd5–0x0aee12). Under additive
blending, a depth-passing primary terrain pixel therefore supplies
its shader RGB with weight one and the previous target RGB with
weight zero. Nearby draws can change a pixel by their own
coverage, depth and blend state.

The first device request uses windowed BackBufferFormat 0
(D3DFMT_UNKNOWN) at 0x0c875c–0x0c87d0. The device Reset path
requests BackBufferFormat 0x15 (D3DFMT_A8R8G8B8) in either window
mode at 0x0c89de–0x0c8a58. The latter stores normalized 8-bit
color channels. Hardware rounding and dithering of shader results
at that target boundary were outside the static trace.

The ordinary application wrapper calls the game's render slot at
0x0c86ec, then calls Present on that same Direct3D device with
all four optional arguments null (0x0c86ef–0x0c86ff). The
gameplay draw ends its scene at 0x0bbb29. The traced ordinary
path has no terrain-specific full-frame shader between the
draw and Present. The executable also has render-target
save, switch and restore helpers at 0x28d290 and 0x28c770.
Their availability does not establish a composition pass for
the ordinary terrain draw.

## Verification limits

The shader register bindings and arithmetic derive from the
embedded tokens and matched original CPU instructions. An
independent arithmetic model matched a token trace on 192
synthetic vertex inputs and 192 pixel inputs, including the
optional modulation and the fog transitions. This establishes
the formulas at the shader arithmetic level. Device-specific
instruction and interpolation precision, degenerate light
vectors, and visual acceptance remain unresolved.

The traced device setup, indexed terrain pass and Present path
showed no explicit sRGB-write state or gamma-ramp change.
The analysis does not establish all inherited device states,
secondary shader or render-target paths, or the final desktop
and display color processing.
