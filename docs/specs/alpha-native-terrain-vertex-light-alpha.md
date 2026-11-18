# Native terrain packed vertex light

**FINAL / STABLE / HELD**

## Supported image and scope

This contract covers the original 32-bit Alpha `Cube.exe` with SHA-256
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs relative to image base `0x00400000`. The traced
route is the ordinary upward face of a stored solid terrain cell,
from light preparation through the primary indexed terrain draw.

## Stored cells and effective samples

The world has a 1,024 by 1,024 table of 32-bit region pointers at
`+0xbc`. Divide whole X and Y by 16,384 to get region indices *i* and *j*,
then select pointer index `1024i + j`. Whole X and Y are valid from
0 through 16,777,215. Area indices are whole X and Y divided by 256.
Within a region, the area pointer table begins at `+0x10018` and
uses index `64 × (areaX mod 64) + (areaY mod 64)`. The column
address is the pointer at area `+0xa8` plus
`0x20 × ((X mod 256) + 256 × (Y mod 256))`
(`0x0347a0`, `0x034a90`, `0x034d10`).

A column has signed base Z at `+0x10`, a four-byte cell array
pointer at `+0x18`, and count at `+0x1c`. For a stored cell, index
the array by whole Z minus base Z. Cell byte `+3` contains the tag
in its low five bits. The preparation routine edits stored cells
using `0x19aaf0`; vertex sampling uses the read at `0x02f730`.

The terrain sampler at `0x0c1510` uses these effective cells.
Preparation uses the same exceptional samples for its neighbor reads.
The listed bytes are in memory order.

| Query | Effective cell bytes or source |
| --- | --- |
| X or Y outside bounds, missing column, or Z below base | (200, 200, 200, `0x01`) |
| Z at or above base plus count and Z > 0 | (255, 255, 255, `0x00`) |
| Z at or above base plus count and Z ≤ 0 | (255, 255, 255, `0x82`) |
| In range, stored tag zero, Z ≤ 0 and stored flag `0x40` clear | (255, 255, 255, `0x82`) |
| Other in-range query | Stored cell |

The terrain sampler selects these at `0x0c1660`–`0x0c1737`.
Its three default cells are initialized at `0x2f9cc0`,
`0x2f9cf0` and `0x2f9d20`. The corresponding preparation defaults
are initialized at `0x2faad0`, `0x2fab00` and `0x2fab30`.

## Preparation of the stored light byte

In raw cells with tag 0 or 2, byte `+0` holds the light value that
the vertex sampler reads. The helper at `0x19a0e0` prepares it in a
horizontal rectangle. The mesh-builder call passes the default area
argument zero and uses the same rectangle for work and commit.

1. For each existing column, scan stored cells from the highest index
   down. Start with an open column. In stored tag 0 or 2 set both
   bytes `+1` and `+2` to 255 while open, or zero once obstructed.
   The first other stored tag closes the column for the rest of the
   scan. This pass preserves stored byte `+0`
   (`0x19a135`–`0x19a192`).
2. Run sixteen rounds. In each, visit the rectangle's columns in
   increasing X then increasing Y, and each column from bottom
   to top. For stored tag 0 or 2 with byte `+2` below 255, take
   the largest light among the effective neighbors in order −X,
   +X, −Y, +Y, −Z, +Z, stopping at 255. A neighbor of tag 13
   contributes 255. Tag 0 or 2 contributes the greater of 5 and
   its byte `+2`. Other tags contribute zero. Store the integer
   quotient of 85 times that maximum divided by 100 to byte `+1`.
   The 255 entries retain their value
   (`0x19a20a`–`0x19a698`).
3. At the end of each round, copy byte `+1` to byte `+2` for
   every stored tag 0 or 2, after the neighbor phase has finished.
   After the sixteenth round, copy byte `+2` to byte `+0` for
   those tags in the destination rectangle
   (`0x19a6ff`–`0x19a7cd`).

The mesh worker conditionally calls the helper at `0x09db84` before
building the 32-by-32 chunk. A nonzero cache byte `+0x74` selects a
four-cell pad. With that byte clear, an origin at 0 or 224 modulo
256 on either horizontal axis selects a sixteen-cell pad. Other
interior chunks with the byte clear use the existing stored values.
Each pad extends both ends of the half-open chunk rectangle. The
decision is at `0x09db10`–`0x09db89`. Other original callers use
the helper at `0x1ede84`, `0x20f071` and `0x213fc0`.

## Ordinary upward face and corner alpha

The builder at `0x09d910` skips current tag zero in this path.
The ordinary top face can be emitted when the effective cell above
has tag 0, or when it has tag 2 and the current cell has a different
tag (`0x0a01c0`–`0x0a01fe`). The regular stored solid starts
from its first three bytes divided by 255 as base RGB and 1 as base
alpha (`0x09e417`–`0x09e4f1`). The below-column-base color
adjustment and tag-2 tint are additional base-color branches.
The ordinary upward face supplies this base vector to corner
sampling, which computes a new alpha.

The four corners are `(x,y)`, `(x+1,y)`, `(x+1,y+1)` and
`(x,y+1)`, at whole Z `z+1`. The calls at `0x0a062d`,
`0x0a06eb`, `0x0a07a5` and `0x0a0861` pass normal
(0, 0, 1) and a zero bypass byte. A nonzero bypass would return
the supplied four base components directly (`0x0c1516`–`0x0c1554`).

For each corner, visit the 36 cells at that corner's Z and X and Y
offsets −3 through +2, increasing X with increasing Y inside each
X. The sampler selects offsets 0 for a positive normal component
or −1 for a negative component in the corresponding axis. A zero
normal component takes all six offsets (`0x0c1557`–`0x0c165e`,
`0x0c1843`–`0x0c187f`). A sample's weight is `(3 − d) / 3`,
where *d* is the largest of the three absolute offsets after adding
0.5 to each. Compute the weight in binary32.

| Effective sample tag | Intensity used for alpha | Contribution to open weight |
| --- | --- | --- |
| 0 or 2 | Greater of 5 and unsigned cell byte `+0` | Sample weight |
| 13 | 255 | Sample weight |
| Other | 5 | Zero |

The routine starts three binary32 sums at zero. It adds the weight,
the intensity times weight, and the open weight to their respective
sums in sample order. Alpha is the weighted intensity sum divided
by the binary32 product of 255 and the total weight. If total weight
is nonpositive, the initialized zero alpha applies. The independent
ratio of open weight to total weight blends base and adjusted RGB.
Every sample participates in the alpha denominator, including solid
samples that add no open weight
(`0x0c1737`–`0x0c1832`, `0x0c1885`–`0x0c18a8`,
`0x0c19b1`–`0x0c19fe`).

The ordinary face appends six 32-bit index values, relative to the
first new vertex as 0, 1, 2, 0, 2, 3 (`0x0a0588`,
`0x06eeb0`). Current tag 2 selects the alternate index list
(`0x09e9c8`–`0x09eb34`). Current tag 3 with effective tag 0
above emits an extra top quad to that alternate list with base
RGBA (0, 0, 1, 1) and the same alpha computation, then emits the
ordinary top quad (`0x0a01f9`–`0x0a0573`).

## Byte conversion and indexed draw

The packer at `0x066650` multiplies each color lane, including alpha,
by binary32 255, truncates toward zero to signed integer, and
stores the low byte of each result. The operation rounds at the
multiplications and at each sampler step above. The eight-byte vertex
record uses the low bytes of chunk-local X and Y, slab-local Z and a
normal selector at `+0` through `+3`. The outward +Z selector is 4.
The color word at `+4` is `0xAARRGGBB`, stored as B, G, R, A. Its
alpha is at `+7` (`0x066660`–`0x0666f2`,
`0x066807`–`0x06682c`). The append at `0x086830` copies all eight
bytes.

In normal binary32 rounding, a 36-cell layer of uniform effective
intensity 255 produces packed alpha 254. Uniform effective intensity
5 produces 4, and uniform intensity 128 produces 128. Replacing one
of the four nearest 255 samples with intensity 5 produces 238.
Replacing one far corner instead produces 251.

For each nonempty slab the uploader creates a managed write-only
vertex buffer and copies its packed bytes into a lock at offset zero,
then unlocks. It does the same for each nonempty index list using
32-bit indices. The vertex upload is at `0x0a0ab0`–`0x0a0b4e`.
The index uploads are at `0x0a0b51`–`0x0a0bef` and
`0x0a0bf2`–`0x0a0c8a`. The per-slab draw record stores the vertex
buffer at `+4`, primary and alternate index buffers at `+8` and
`+0xc`, vertex count at `+0x10`, respective triangle counts at
`+0x14` and `+0x18`, and slab Z origin at `+0x1c`
(`0x0a1277`–`0x0a12f3`).

The primary draw sets stream 0, offset zero and stride eight, binds
the primary index buffer, then issues an indexed triangle list with
base vertex, minimum vertex and start index all zero
(`0x0b1d63`, `0x0b1d79`, `0x0b1d9a`). Its vertex declaration
uses POSITION0 of type UBYTE4 at `+0` and COLOR0 of type D3DCOLOR
at `+4` (`0x047e25`–`0x047e58`). The binding at `0x0b1a71`
selects the declaration and shaders through `0x047d10`.

The embedded vertex shader at `0x2fd928` obtains normalized vertex
alpha in COLOR0.w. It multiplies that value by `materialColor.w`
at vertex constant `c32` (`0x2fddf4`). The result scales the
front, back and ambient RGB sum before local point-light addition
(`0x2fdf04`–`0x2fdf44`, `0x2fe070`). It also forwards the input
alpha to outgoing COLOR0.w (`0x2feb34`). The primary pixel shader
at `0x2feb50` reads COLOR0.rgb and writes output alpha from its
constant `c3.x` (`0x2fece0`, `0x2fef8c`).

## Verification limits

The cited CPU instructions, constants, declaration and shader tokens
were checked in the matched image. The weighting and byte conversion
matched a scalar trace of the native instructions on five uniform or
single-occluder layers and 256 mixed synthetic 36-cell layers.
Native runtime appearance, the complete origin of every stored cell
tag and cache state, secondary shader behavior, and other mesh
producers remain outside this contract.
