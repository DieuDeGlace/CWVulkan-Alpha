# Alpha renderer fixed coordinates and terrain light inputs

**FULL / FINAL / STABLE / HELD.** This source ruling covers the startup
render reference and the ordinary indexed terrain pass in the x86
`Cube.exe` with SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
RVAs are relative to `0x00400000`. Evidence comes from the original
instruction bytes and embedded shader data.

## Fixed coordinates at the render boundary

- The game constructor stores a three-axis reference at game offsets
  `+0x140`, `+0x148`, and `+0x150` (RVAs `0x059cf9` through
  `0x059d73`). The axes are signed 64-bit fixed values with 16 fraction
  bits. Their initial raw values are `0x27100000`, `0x27100000`,
  and `0x003c0000` with zero high words. This is `(10000, 10000, 60)`
  in whole coordinate units. World state and camera updates can move
  the reference after construction.
- On the traced frame path, the client adds the signed horizontal scene
  offsets at game `+0x1d8` and `+0x1e0` to the fixed X and Y
  reference axes. It adds zero to Z. The additions are performed on the
  full fixed values before conversion (RVAs `0x0ad840` through
  `0x0ad8b1`, addition helper `0x02c800`). Each resulting signed
  integer is rounded to binary32, then multiplied in binary32 by
  `2^-16` from RVA `0x2fcd94`. The result is uploaded as XYZ with
  W zero to vertex constant `c36`, named `cameraPosition` (call at
  `0x0ad92c`, uploader `0x048010`). The frame update writes the
  horizontal offsets from negated fixed values at `0x09cd09` through
  `0x09cd2c`.
- In the indexed terrain draw, each horizontal key in the region record
  at `+0x18` is shifted left five bits as a 32-bit value. It is then
  sign extended into the fixed coordinate scale, added to the matching
  scene offset, and converted with the same signed integer to binary32
  to `2^-16` sequence. The vertical translation is a direct conversion
  of the signed integer at `+0x1c` of that region's selected mesh entry.
  This supplies the terrain world transform (RVAs `0x0b1c7c` through
  `0x0b1d40`, helpers `0x012080`, `0x0122c0`, and `0x0120f0`).
  The shader compares transformed world position with rebased
  `cameraPosition` in the same coordinate space (RVA `0x2fdd4c`).

## Primary terrain light inputs

The terrain pass binds the world vertex shader at RVA `0x2fd928` and the
pixel shader at `0x2feb50` (setup `0x047e10`, binding call `0x0b1a71`).
The indexed draw uses an eight-byte vertex stride. The declaration
supplies four unsigned bytes at offset zero and normalized packed color
at offset four. The first three position bytes are local XYZ. The fourth
selects a face normal. Values 0 through 4 correspond to +X, −X, +Y,
−Y and +Z respectively, with −Z as the fallback (RVA `0x2fde04` through
`0x2fde94`).

- The world state constructor initializes its signed millisecond clock at
  `+0x80015c` to `43,200,000` (RVA `0x18ede5`). The frame reads
  that same field as game `+0x800440`, since the world state begins at
  game `+0x2e4`. The color producer at `0x19d640` uses the clock and
  the unshifted fixed reference (call at `0x0ad7a1`). It computes the
  absolute value of `2t / 86,400,000 − 1` with binary32 time arithmetic,
  cubes it in binary64, rounds to binary32 and subtracts from one, then
  raises that result to the fourth power in binary64 and rounds to
  binary32 for the color schedule (RVAs `0x19d65b` through `0x19d708`).
  The color outputs also depend on location and sampled cell data.
- The frame computes direction from the same clock. It doubles the
  clock value in binary32, multiplies by binary64 π, divides by
  `86,400,000` in binary64, and rounds the angle to binary32. The
  direction before normalization is X zero, Y negative sine, Z negative
  cosine, with sine and cosine evaluated in binary64 and rounded to
  binary32 (RVAs `0x0ad7a6` through `0x0ad822`). The terrain upload
  at `0x0b1a9c` normalizes it on the CPU and sends it to vertex
  `c33`. The color producer's fourth, fifth and sixth color outputs
  feed `c34` `lightFrontColor`, `c35` `lightBackColor`, and `c38`
  `ambientColor` respectively at this call. The separate sky and fog
  outputs go to pixel `c0` through `c2` (call at `0x0ad9cc`).
- The vertex shader forms the base illumination from ambient and the
  nonnegative front and back normal-dot terms. It weights that base with
  the product of the mesh color alpha and `c32` material alpha
  (RVAs `0x2fddf4`, `0x2fdee4` through `0x2fdf44`, and
  `0x2fe070`). Point-light terms add to the weighted base. The shader
  clamps the combined illumination per channel to the range 0 through
  1.2 before multiplying by the mesh and material RGB
  (`0x2fea60` through `0x2feaa0`). The separate darkness contribution
  has zero RGB for this terrain pass (`c37`, call at `0x0b1aef`).
- At frame setup the client supplies zero point-light banks
  (`0x0adaff`). At `0x0b1c0b`, it instead supplies entries from the
  selected terrain region record at `+0x78` for position and positive
  radius and `+0x178` for RGB. The uploader at `0x048f10` sends ten
  float4 positions to vertex `c0` through `c9`. It expands 16 CPU
  float3 colors with zero W and sends the first ten to `c10` through
  `c19`. In the shader a positive radius enables the slot. Its distance
  term squares the remaining fraction after clamping the squared
  distance over squared radius at one. The native frame restores the
  zero banks after the region loop at `0x0b21c2`.

## Boundary with model rendering

The later world creature model path at `0x0b5db2` receives the same
horizontal scene offsets and the shared shader binding. It separately
uploads a creature field at `+0x1184` to vertex `c39`, named
`white` (call at `0x0b5b5d`). That input blends the lit RGB toward
white in the vertex shader (`0x2fead4`). A separate model caller
supplies zero local point banks and preset directional and ambient
colors before invoking the same model routine (RVAs `0x025e46`,
`0x025ff7`, and `0x0269e1`).

This trace establishes the initial reference, conversion order and one
terrain light input path. The full trigger for updating the horizontal
reference, the full environment color schedule and its sample
semantics, point-light producer selection, the source of mesh RGB and
alpha, and the light state of all object and creature passes remain
open.
