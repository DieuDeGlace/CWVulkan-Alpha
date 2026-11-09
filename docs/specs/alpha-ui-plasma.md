# Alpha interface: the Plasma `.plx` files, their runtime and the fonts

Behavioural specification of the interface library ("Plasma", by Picroma) built into the Cube
World **Alpha** client (`Cube.exe`, SHA-256 `84a7a132…30bf`), and of the five interface files it
loads from the install folder: `gui.plx`, `start.plx`, `help.plx`, `cursor.plx`, `quest-tag.plx`,
plus the two fonts `resource1.dat` and `resource2.dat`. Written from the Alpha decompile
(`disassembled alpha cube world/decompiled/client`), its assembly and the bytecode of the
interface shaders in the executable, and **checked against the files themselves**: every
statement in sections 2–5 marked *(verified)* was confirmed by walking all five files from the
first to the last byte with a reader built from this description.

Routines are cited by address in `Cube.exe` (image base 0x400000). "G" is the game object (as
in `alpha-rendering.md`). How the game uses the files (which nodes it clones, where it puts
them, what each screen does) is in `alpha-ui-screens.md`.

---

## 0. Conventions

* Little-endian. "u32"/"i32" 32-bit integers, "f32" IEEE-754 single. No alignment or padding
  anywhere in the files.
* "wstring" = u32 character count *n*, then *n* UTF-16LE code units (no terminator).
  "string" = u32 byte count *n*, then *n* bytes.
* Page units: one unit is one window pixel at run time (the game positions interface nodes in
  window pixels, `alpha-ui-screens.md` §2). The y axis points **down** (larger y is lower on the
  screen).
* Colours are RGBA floats 0..1.

---

## 1. The files

| File | Size | Holds | Loaded into (0x459c40) |
|---|---:|---|---|
| `gui.plx` | 822,138 | every HUD/window template (355 nodes, 4 textures, 24 widgets, 12 buttons, 1 edit) | the in-game interface root (G+0x800884) |
| `quest-tag.plx` | 75,421 | the "QUEST COMPLETE!" banner `questtag` and the small `smallquesttag` | the same root |
| `help.plx` | 258,161 | the keyboard help picture (node `help`) | its own container (G+0x80089c), toggled with F1 |
| `start.plx` | 1,534,507 | the title logo `cubeworld` and the `picroma` logo | the start-screen root (G+0x800874) |
| `cursor.plx` | 11,609 | a tooltip/"ACTION" label sample | loaded at 0x461e46 |

Each file is one Plasma "document": a flat stream of top-level records (§2) that create
textures, shapes, transformations, displays, widgets and nodes, then link nodes into a tree by
index. The game then finds subtrees **by node name** and clones them (§6.6).

---

## 2. The container format (verified)

### 2.1 Records and names

The whole file, and the inside of every object, is a sequence of **records**:

1. a **name token** (u32);
2. a **payload size** (u32, bytes);
3. the payload.

A name token is a number standing for a text name. Names are defined in-line the first time
they are needed: if the u32 read where a token is expected is **0**, a definition follows:

* u32 size of the definition (bytes, always 8 + length of the name);
* u32 the new token number (1, 2, 3, … in order of definition);
* string: the name's bytes (§2.2 for the scrambling);
* then the u32 that is the actual token of this record (normally the number just defined).

Definitions can occur anywhere a token is expected, including deep inside nested payloads
(e.g. the token for `Attribute.frame` is first defined inside the first shape's `texture`
property). Tokens are global for the file; a reader must therefore walk the file in order
and descend into nested payloads in order. (Reader: 0x688220; plain name read 0x688510.)

Every payload is self-delimiting by its size, so a reader can skip any record it does not
understand (the game does: unknown top-level records and unknown properties are skipped,
0x6886f0).

### 2.2 Header, seal and name scrambling

A file always starts with these two records (bytes identical in all five files):

1. Token 1 defined as `PlasmaGraphics`; payload 4 bytes: **format version = 1**. A version
   above 1 is refused ("invalid version", 0x681fc5).
2. Token 2 defined as `Seal`; payload: string of 14 bytes. This is a licence seal.

From the seal on, **every name definition's bytes are scrambled**. To read them:

* The key *K* is a 64-bit number. Start with h = 0x0003FFFF_FFFFFFE5 and for each byte *c* of
  the text `PlasmaXGraphics` do h = h·31 + c (64-bit wrap-around). *K* = h =
  **0xFB3A260B5EC7CD5C** (computed at 0x687b10; the game registers this key at start-up,
  0x459c40 near the `PlasmaXGraphics` text).
* Let k₀…k₇ be the bytes of *K* in little-endian order, *v* = the low 32 bits of *K*, *n* = the
  length of the scrambled name and *r* = *v* mod *n*. For every input index *i* (0 ≤ *i* < *n*):
  output position *d* = (*i* + *r*) mod *n*; output[*d*] = (input[*i*] − k[*d* mod 8]) mod 256
  (0x6880c0).
* The seal check (0x681c70): the 14 seal bytes, unscrambled the same way, must read
  `PlasmaGraphics`. The game tries every key it knows (the registered licence keys, then 0, then
  the hash of `PlasmaGraphics`) and adopts the first that passes; otherwise it refuses the file
  ("invalid demo licence"). In all five files the passing key is the one above. An
  implementation can simply use that key; it need not check the seal.

Names defined **before** the seal (`PlasmaGraphics`, `Seal`) are plain. Name definitions are the
only scrambled data; all values are plain.

### 2.3 Property values

Inside an object payload every record is a **property** whose name is `<Kind>.<field>`
(e.g. `Node.shape`). The payload encodings are:

| Encoding | Payload |
|---|---|
| int | i32 |
| float | f32 |
| raw wstring | wstring (e.g. `*.wname`, `TextShape.wfontName`) |
| raw int array | u32 count *n*, then *n* u32 (`face`, `vertexFlags`, `vertexParameters`) |
| **attribute** | a list of `Attribute.frame` records, optionally followed by `Attribute.sequence` records (§2.4) |
| **array attribute** | one `ArrayAttribute.size` record (i32 element count *m*), then `ArrayAttribute.frame` records each holding *m* elements, then optional `Attribute.sequence` records |

An `Attribute.frame` payload is one value of the attribute's type: f32, i32, 2×f32 (vector2),
3×f32 (vector3), 4×f32 (colour), 16×f32 (4×4 matrix, row-major, translation in the last row),
or a wstring (text). An `ArrayAttribute.frame` payload is *m* consecutive values.

### 2.4 Frames and sequences (animation data)

Every animatable property stores **frames**: frame 0, frame 1, and possibly more.

* **Frame 0** is the authored value. In the shipped files frame 0 always holds sensible data.
* **Frame 1** is the library's "current value" slot (the evaluator writes its result there,
  0x66a950, 0x66b0c0). In the files it is either a copy of an editor state or left-over memory
  (matrices often contain garbage). **Do not use frame 1 from the file**; initialise the current
  value from frame 0 (open question P-1).
* **Frames 2…** are keyframe values used by sequences.

An `Attribute.sequence` record contains:

* `Attribute.sequence.wname` (wstring): the sequence name, e.g. `shine`, `show`, `show2`, `hit`,
  `button:enter`.
* one `Attribute.sequence.key` record per key, in time order, each containing
  `Attribute.sequence.key.frame` (i32: which frame of this attribute holds the key value),
  `Attribute.sequence.key.time` (i32 milliseconds from the start of the sequence) and
  `Attribute.sequence.key.smoothness` (f32, 0..1; seen values 0 and 1).

A sequence with the same name may exist on many attributes of many objects; playing a sequence
on a node subtree plays every attribute sequence of that name in it (§6.5).

### 2.5 Top-level records

| Record | Payload | Index space |
|---|---|---|
| `PlasmaGraphics` | i32 version (1) | — |
| `Seal` | string (14 bytes) | — |
| `Texture` | properties §3.1 | looked up by its `Texture.id` |
| `SmoothMeshShape` | properties §3.2 | **shape** list |
| `TextShape` | properties §3.3 | **shape** list |
| `GenericShape` | (not present in the shipped files; read by 0x683870) | **shape** list |
| `Transformation` | properties §3.4 | **transformation** list |
| `Display` | properties §3.5 | **display** list |
| `Widget`, `Button`, `ListWidget`, `Edit`, `ScrollButton`, `ScrollSlider`, `PopUpButton` | properties §3.6 | one shared **widget** list |
| `Node` | properties §3.7 | **node** list |
| `pageWidth`, `pageHeight`, `dpi` | f32 | document page (editor information) |
| `unit` | i32 (0) | — |
| `pageColor` | 4×f32 | — |

Each list is numbered 0, 1, 2… **in file order, per list** (the three shape kinds share one
counter; the seven widget kinds share one counter). References from nodes use these numbers.
The loader dispatches on the record name (0x681c70); within each object the property order in
the file is fixed but a reader should accept any order and skip unknown properties.

Page values in the files: `gui.plx` 760 × 530, `start.plx` 1600 × 1000 (dpi 235.93),
the others 1000 × 1000, dpi 72, page colour (1, 1, 1, 0). The page is editor information only;
the game ignores it.

---

## 3. The objects (verified field lists)

Field types: see §2.3. "attr" = attribute with frames; "array attr" = array attribute. Counts
are over all five files.

### 3.1 `Texture`

| Field | Type | Notes |
|---|---|---|
| `wname` | wstring | `star1.png` … `star4.png` (the only four textures, all in `gui.plx`) |
| `id` | int | the number shapes use to refer to it (2, 4, 5, 3 for star1, star2, star3, star4) |
| `format.pixelFormat` | int | 1 in all files: 8-bit RGBA |
| `format.minFilter`, `format.maxFilter` | int | 0 |
| `format.horizontalWrap`, `format.verticalWrap` | int | 0 |
| `width`, `height` | int | 50 × 50 (star4 52 × 50) |
| `compressedPixels` | u32 size *s*, then *s* bytes of a **zlib** stream | inflates to width × height × 4 bytes, rows top to bottom (0x686820 → 0x449540) |
| `pixels` | uncompressed variant (not used in the files) | |

Byte order of a pixel: believed R, G, B, A (star4's pixels read FD 82 00 FF, an orange). Open
question P-2.

### 3.2 `SmoothMeshShape` (321 shapes)

A **polygon mesh** in the plane: shared vertices, polygon faces (mostly quads, also long
outline polygons), per-vertex colours, optional stroke along the outline, optional subdivision.

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | shape name (editor name; many are `procedural:preview`, meaningless) |
| `face` | raw int array, **repeated once per face** | the face's vertex indices in order |
| `vertexFlags` | raw int array (one per vertex) | corner type, low 16 bits 0–3 (§4.2); 0x10003 also occurs (high bits: open question P-3) |
| `vertexParameters` | raw int array (one per vertex) | mostly 0; otherwise f32 bit patterns 5, 8, 10, 15 (meaning unknown, P-3) |
| `vertexPositions` | array attr of vector2 | |
| `vertexTexCoords` | array attr of vector2 | 0..1 |
| `vertexColors` | array attr of colour | fill colour per vertex |
| `strokeColors` | array attr of colour | stroke colour per vertex |
| `extrusionFrontColors`, `extrusionBackColors` | array attr of colour | used only with flag 16 |
| `strokeWidths` | array attr of f32 | stroke width per vertex (page units) |
| `texture`, `strokeTexture` | attr of int | `Texture.id`, or −1 for none (only the four star shapes use a texture) |
| `textureTranslation`, `texturePivot` | attr vector2 | texture placement |
| `textureRotation` | attr vector3 | degrees |
| `textureDeformation` | attr matrix | |
| `textureOpacity`, `textureBrightness`, `textureContrast`, `textureSaturation` | attr f32 | all 1 in the files (§5.2) |
| `strokeTextureOpacity`, `…Brightness`, `…Contrast`, `…Saturation`, `strokeTextureStretch` | attr f32 | all 1 |
| `extrusionMatrix` | attr matrix | identity |
| `subdivisions` | int | 0–4 in the files (the library clamps to 0–6, 0x642a50); 202 shapes use 0 |
| `flags` | int | bit 1 (2) = draw the stroke; bit 2 (4) = **no fill**; bit 4 (16) = extrusion; bit 0 (1) meaning open (P-4). Values seen: 0, 1, 2, 3, 6, 14, 17 |
| `strokeJointType` | int | 0 or 1 (1 = round joins, P-4) |
| `strokeCapType` | int | 0 or 1 |
| `strokeAlignment` | int | −1, 0, 1 (0 = centred on the outline; ±1 = inside/outside, P-4) |
| `strokePattern` | int | 0 (solid) everywhere |
| `strokeDash`, `strokeGap` | float | 1 and 2 (unused with pattern 0) |
| `smoothWeight` | float | 0.585786 = 2 − √2 everywhere (the library default, 0x63c2b0) |

### 3.3 `TextShape` (194 shapes)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | node-ish name (`text`, `info`, `name`, `count`, `class`, `objective`, …) |
| `string` | attr of wstring | the text; the game replaces it at run time |
| `color` | attr colour | fill colour of the glyphs |
| `strokeColor` | attr colour | outline colour |
| `extrusionColor` | attr colour | unused (0.5 grey) |
| `flags` | int | layout flags, §4.4 (0, 1, 2, 4, 5, 6, 17, 21 seen) |
| `wfontName` | wstring | `resource1.dat` (190 shapes) or `resource2.dat` (4: the land name banner) |
| `pixelSize` | float | glyph size in page units: 8, 9, 10, 12, 14, 16, 20, 22, 40, 69 |
| `strokeRadius` | float | outline radius in page units (0 = no outline; 2–7 seen) |
| `spacing` | float | extra letter spacing (0 everywhere) |
| `lineSpacing` | float | extra line spacing (0, or 2) |
| `wrapWidth` | float | wrapping width (200 by default; 258 for quest objectives, 400 for land names) |

Most labels exist as **two text nodes with the same name at the same place**: the first has an
outline (`strokeRadius` > 0, often black or dark), the second none. The game writes the same
text into both. Draw them in order and the second covers the inside of the first.

### 3.4 `Transformation` (626 objects)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring (empty) | |
| `translation` | attr vector2 | position of the node in its parent's space |
| `rotation` | attr vector3 | degrees; only z is used (seen 90 and 270) |
| `pivot` | attr vector2 | the point of the node that `translation` places |
| `deformation` | attr matrix | linear part (scale/shear), row-major, identity in most |

### 3.5 `Display` (601 objects)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring (empty) | |
| `visibility` | attr int | 1 shown, 0 hidden (hides the whole subtree). The game opens and closes windows by toggling this value (0x488bd0, 0x488c00, 0x488d00) |
| `clipping` | attr int | 1 = this node's shape is a **mask** for its children (§5.3); 10 nodes use it |
| `fillColor` | attr colour | multiplies the colours of the node and its subtree (§5.1) |
| `strokeColor` | attr colour | multiplies stroke colours |
| `blurRadius` | attr f32 | 0 everywhere (blur pass §5.4) |
| `flags` | int | 0; 2 on mask nodes (7 nodes); 4 on one mask node in `start.plx` (P-5) |

### 3.6 `Widget`, `Button`, `Edit` (37 objects in `gui.plx`)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | e.g. `lifebar`, `button`, `edit`, `itembox` |
| `caption` | wstring (empty) | |
| `innerBindPos`, `innerBindSize` | vector2 | the stretchable inner rectangle (§5.5) |
| `bindPos`, `bindSize` | vector2 | the widget's rectangle as drawn in the file |
| `framePos`, `frameSize` | vector2 | equals bind rectangle in the files |
| `bindMatrix` | 16 f32 | the widget's placement matrix at bind time |
| `horizontalAlignment`, `verticalAlignment` | int | 0, or 2 (3 widgets) |
| `flags` | int | 0, or 4 (2 widgets) |
| `Button.type` | int (buttons only) | 0 |

(These fields are plain values, not attributes.)

### 3.7 `Node` (623 objects)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | the name the game searches for |
| `shape` | int | index into the shape list, −1 = none |
| `transformation` | int | index into the transformation list |
| `display` | int | index into the display list |
| `widget` | int | index into the widget list, −1 = none |
| `child` | int, **repeated once per child** | child node indices, in drawing order |
| `flags` | int | 0x2000 on node 0 of every file; 0x800 or 0x880 on some groups (`questtag`, `smallquesttag`, `combopoints`, `cubeworld`, `picroma`, `help`, groups in `start.plx`) (P-6) |
| `Node.variable` | two strings | not present in the files |

**Node 0 of every file is the document root and is not created**: the loader attaches node 0's
children to the container it is loading into, and node 0's own shape, transformation and
display are ignored (0x683f00 with the "first node" flag). Its transformation in the files is
the editor's view (scale 2.25 in `gui.plx`) and must not be applied.

The same shape may be referenced by several nodes (e.g. the four `star` shapes are used twice).

### 3.8 Statistics of the shipped files

| | gui | start | help | quest-tag | cursor |
|---|---:|---:|---:|---:|---:|
| Nodes | 355 | 66 | 168 | 25 | 4 |
| SmoothMeshShapes | 199 | 65 | 39 | 17 | 1 |
| TextShapes | 113 | 0 | 72 | 7 | 2 |
| Transformations / Displays | 338 / 338 | 66 / 66 | 168 / 168 | 25 / 25 | 4 / 4 |
| Widgets + Buttons + Edits | 37 | 0 | 0 | 0 | 0 |
| Textures | 4 | 0 | 0 | 0 | 0 |

`start.plx` is large because its letters (C, U, B, E, W, O, R, L, D, "Picroma", "©2013") are
dense meshes with up to 228 faces. Only `quest-tag.plx` and `gui.plx` (`combopoints`) contain
sequences.

---

## 4. Geometry

### 4.1 Transform of a node

For a node with translation **t**, pivot **p**, rotation angle θ (the z value, degrees) and
deformation matrix **D** (its 2×2 linear part), a point **x** of the node's shape maps into the
parent's space as

**x′ = t + R(θ) · D · (x − p)**

and the node's full transform is the product of its ancestors' transforms (root first). The
game uses exactly this to anchor nodes: it writes translation = target point (and treats the
pivot as the node's hot spot, `alpha-ui-screens.md` §2). The order of R and D and the sense
of θ were not traced (open question P-7: verify with `upbutton`/`downbutton`, whose
untransformed shape is the same arrow as `leftbutton`, rotated 90° and 270°; the rendered
result must point up and down respectively).

### 4.2 The smooth mesh

**Control mesh.** Vertices are `vertexPositions` (current value), faces are the `face` lists.
Faces may share vertices and edges. Example: `leftbutton` has 8 vertices and three quads
forming an arrow band; `graywidget` is one 12-corner polygon (a rectangle with notched
corners).

**Subdivision** (`subdivisions` = *L* > 0). The mesh is refined *L* times. For the outline loops
the library refines every loop of vertices with the following rule (0x63ad70, run for positions,
texture coordinates and, with the matching variants 0x63b360 / 0x63a980 / 0x63bba0, for colours,
stroke colours, stroke widths and extrusion colours). Each pass, with *w* = `smoothWeight` and
corner type *f* = low 16 bits of `vertexFlags`:

1. **Insert a midpoint** on every edge a→b: m = ½(a + b). On the *first* pass only, if exactly
   one end has type 3, the point is moved towards the other end: with c = 1/(1 + 2·cos 45°) =
   0.414214, m = c·a + (1 − c)·b when *a* is the type-3 end, m = (1 − c)·a + c·b when *b* is.
   The new point's type is 2 if either end has type 2 (or, on the first pass, type 3), else 0.
2. **Move the old vertices** (all of a closed loop; all but the two ends of an open one), using
   the two new neighbouring midpoints m₋ and m₊: type 1 stays; type 2 becomes ½(m₋ + m₊);
   type 3 becomes ½(m₋ + m₊) on the first pass and stays afterwards; type 0 moves to
   v + w·(½(m₋ + m₊) − v).
3. **Straighten**: for every edge whose two (updated) ends both have type 2 (or, first pass,
   type 3) the midpoint is reset to ½(a + b) and its type becomes 1.

So: type 0 = smooth curve point, 1 = sharp corner, 2 = straight-segment point, 3 = rounded
45° corner. With *w* = 2 − √2 smooth closed loops converge towards circles through the control
polygon. How the interior of a multi-face mesh is refined (the half-edge/quad-face structure
built by 0x63fec0 / 0x644fa0 with `QuadVertex`, `QuadEdge`, `QuadFace`) was **not** traced
(open question P-8). For the 202 shapes with *L* = 0 none of this applies.

**Fill.** Each face is filled as a polygon with the per-vertex `vertexColors` interpolated
linearly across it (Gouraud). Non-convex outline polygons (`frame`, `graywidget`, the letters)
must be triangulated properly (any constrained/ear-clipping triangulation; the library's own
uses its quad-edge structure). Shapes with flag 4 have no fill. Fill rule for overlapping faces:
faces are drawn independently in face order (P-8).

**Stroke** (flag 2). A band of width `strokeWidths` (interpolated along the outline) around the
**boundary edges** of the mesh (edges used by only one face), coloured with `strokeColors`,
round joins and caps for type 1. Alignment 0 centres the band on the outline; 1 and −1 put it
inside or outside (P-4).

**Texture** (only `star1`–`star4`): texture coordinates from `vertexTexCoords`, transformed by
the texture translation/rotation/deformation/pivot; sampled colour combined as in §5.2.

### 4.3 Anti-aliased edges

The library adds a thin fringe to outlines instead of multisampling: each fringe vertex carries
two edge normals and is pushed outward by `aaOffset` pixels along their bisector divided by the
cosine between bisector and normal (at least 0.7), and pushed back in depth by 0.5 (widget
vertex shader 0x720558). An implementation may instead render the interface with MSAA or its
own edge AA; the visual target is soft 1-pixel edges.

### 4.4 Text layout (TextShape)

* **Font**: the TrueType file named by `wfontName`, read from the install folder (§7). Glyphs
  are rendered at `pixelSize` page units (= pixels).
* **Flags** (inferred from use, P-9): bits 0–1 horizontal alignment relative to the node origin
  — 0 left, 1 centre, 2 right; bit 2 (4) vertical centring; bit 4 (16) wrap at `wrapWidth`.
  Examples: item counts use 2 (right); names and button captions 1 or 5 (centred); quest
  objectives 17/21 (wrap at 258); the left equipment label 6 and the right one 4.
* **Origin = baseline of the first line**; text rises above it (layout 0x65c040, measuring
  0x65d530; details in `alpha-ui-screens.md` §3.4). Bit 4 word-wrap confirmed at 0x65bc70.
* Lines break on explicit line breaks and, with bit 4, at spaces before `wrapWidth`; each
  further line starts `pixelSize` + `lineSpacing` lower; letter advance from
  the font plus `spacing`; kerning from the font's `kern` table (resource1 has one).
* **Outline**: a glyph outline of radius `strokeRadius` drawn in `strokeColor` under the fill.
  The library has a "glow" outline font path (`TextShape.font.glowRadius`, `strokeGlow`); a
  soft outline (distance-field or dilated alpha) of that radius matches.
* **Colour**: `color` × display fill colour (§5.1).

---

## 5. Drawing

### 5.1 Order and colour inheritance

The tree is drawn depth-first: a node's own shape, then its children in `child` order (later
children on top). A hidden node (visibility 0) hides its subtree. The display `fillColor`
multiplies the vertex/glyph colours of its node and (inherited multiplicatively) of all
descendants; `strokeColor` does the same for strokes (P-10 for the exact inheritance). All
interface drawing is alpha-blended over the 3D frame, after it (`alpha-rendering.md` §1.3
step 15), without depth testing.

### 5.2 The widget pixel shader (0x720e28, verified from bytecode)

Constants: `textureEnabled` (bool), `textureOpacity` *o*, `textureBrightness` *b*,
`textureContrast` *k*, `textureSaturation` *s*, `filter` *F*; samplers `textureSampler` and
`maskSampler`; inputs: vertex colour **v**, texture coordinate, mask coordinate.

1. Without texture: colour = **v** × *k* (the contrast constant, normally 1).
2. With texture: sample **t** (RGBA).
   * Saturation, unless *s* = 1: rgb ← lerp(rgb, L, 1 − *s*), L = 0.3R + 0.59G + 0.11B.
   * Brightness, unless *b* = 1: rgb ← rgb × *b*.
   * Contrast, unless *k* = 1: rgb ← lerp(rgb, 0.5, 1 − *k*).
   * rgb ← min(rgb, 1). Alpha stays the texture alpha.
   * colour = **v** × lerp(1, **t**, *o*) (all four channels).
3. Mask filter *F* (from the clipping node, §5.3), with mask sample **m** whose rgb is
   un-premultiplied (divided by its alpha when alpha > 0):
   * *F* = 0: no mask;
   * *F* = 1: rgb ← lerp(rgb, **m**.rgb, **m**.a), alpha unchanged (tint);
   * *F* = 2: rgb ← rgb × **m**.rgb, alpha ← alpha × **m**.a (multiply);
   * any other value: rgb unchanged, alpha ← alpha × **m**.a (pure alpha mask).

The plain textured shader (0x721420) un-premultiplies the texture and multiplies by the vertex
colour: off-screen layers are stored premultiplied.

### 5.3 Masks (clipping nodes)

A node whose display has `clipping` = 1 is a mask: its own shape is rendered into a mask layer
(not shown), and its descendants are drawn through it with the mask filter of §5.2 — which
filter value corresponds to `Display.flags` 2 and 4 is open (P-5; the natural reading is
flags 2 → alpha mask, so that e.g. the quest banner's `shine` highlight and the land-name
banner are cut to the banner's shape). The mask coordinate is the descendant's position
transformed by the mask layer's matrix (`MaskMatrix`, vertex shader 0x720558).

### 5.4 Blur layers

A display with `blurRadius` *r* > 0 is drawn into an off-screen layer, which is blurred with a
separable 9-tap filter: weights 0.18 at the centre and 0.15, 0.12, 0.09, 0.05 at offsets ±1…±4
× `blurScale`, with `blurScale` = 0.25 × *r* / layer size (0x68cad0, shaders 0x722270 / 0x721f00),
optionally after 2×2, 3×3 or 4×4 box down-sampling (0x721530, 0x721778, 0x721a98). No node in
the shipped files has *r* > 0, so this can be implemented last.

### 5.5 Widget stretching (9-slice)

When the game resizes a widget node to a new size (e.g. buttons to 300 × 40, windows to their
panel sizes), its geometry is deformed by the widget vertex shader (0x720558) rather than
scaled. With the bind rectangle (position **B**, size **S**) and the deformed rectangle
(position **B′**, size **S′**), each vertex **p** (in widget bind space, after `bindMatrix`) moves to

**p′ = p + (B′ − B) + u ⊙ (S′ − S)**, with uᵢ = 0 if pᵢ < Bᵢ, 1 if pᵢ ≥ Bᵢ + Sᵢ, else
(pᵢ − Bᵢ)/Sᵢ

per axis, then back through the inverse bind matrix. Geometry left of / above the rectangle
keeps its offset, geometry right of / below it moves with the far edge, and geometry inside is
stretched — a nine-slice. Which rectangle is passed as **B**/**S** (the widget's
`innerBindPos/Size` or `bindPos/Size`) is open (P-11); the inner rectangle gives the expected
look (corners keep their shape).

### 5.6 Interface shaders (for reference)

Created by 0x68a350: widget vertex shader 0x720558 (vs_3_0: `Proj`, `WorldView`, `NormalMatrix`,
`MaskMatrix`, `TextureMatrix`, `widgetBindMatrix`, `inverseWidgetBindMatrix`, `widgetBindPos`,
`widgetBindSize`, `deformedWidgetPos`, `deformedWidgetSize`, `widgetDeformationEnabled`,
`baseColor`, `aaOffset`); widget pixel shader 0x720e28; pass-through vertex shader 0x7212c0;
plain textured pixel shader 0x721420; box down-samplers 0x721530 (2×2), 0x721778, 0x721a98;
blur x 0x722270 and y 0x721f00. The vertex colour is multiplied by `baseColor`; positions get a
fixed −0.135-pixel offset in x and y before projection (D3D9 pixel-centre alignment — omit in
Vulkan).

---

## 6. Runtime behaviour the game relies on

### 6.1 Loading

`load(file, container, flags)` (0x653770, flags 0x20 from the game): parse §2, build every
object, link nodes, attach node 0's children to *container*. Loading never touches the page
records.

### 6.2 Current values

Every attribute has a current value used for drawing (initialised from frame 0, P-1). The game
changes interface state by writing current values: visibility (windows), translation (anchoring,
bar positions), text strings, colours, vertex positions of bars (to show fill levels).

### 6.3 Node lookup

`find(container, name)` (0x633d70 / 0x636040) returns the first node with that `wname` in a
depth-first search. Names are not unique (`text`, `info`, `bar` occur in every bar); the game
looks them up **within** a cloned template.

### 6.4 Cloning

`clone(node)` (0x6326d0) deep-copies a subtree (nodes, transformations, displays, and per the
use of shared shapes, shapes by reference unless the game edits them, P-12). Every HUD element
is a clone of a `gui.plx` template.

### 6.5 Playing a sequence

The game plays a named sequence on a subtree (e.g. `shine` on the quest banner, 0x490711;
`show` at 0x48d259; `hit` and `button:enter` on `combopoints`; the button states
`button:press`, `button:release`, `button:enter`, `button:leave` and their `:checked` variants
on buttons, 0x664d60). For every attribute that has a sequence of that name, at time *t*
milliseconds after the start (0x662300):

* no keys: nothing; one key: current = that key's frame;
* *t* before the first key: current = first key's frame;
* *t* after the last key (non-looping): current = last key's frame;
* otherwise find keys *i*, *i*+1 with tᵢ ≤ *t* ≤ tᵢ₊₁, u = (*t* − tᵢ)/(tᵢ₊₁ − tᵢ), and with the
  neighbouring key values P₀ (key *i*−1), P₁ (key *i*), P₂ (key *i*+1), P₃ (key *i*+2) and the
  smoothness values s₁ (key *i*) and s₂ (key *i*+1), evaluate the cubic Bézier
  **value = (1−u)³·P₁ + 3u(1−u)²·C₁ + 3u²(1−u)·C₂ + u³·P₂** with
  C₁ = P₁ + ¼[s₁(P₂ − P₀) + (1 − s₁)(P₂ − P₁)] and
  C₂ = P₂ − ¼[s₂(P₃ − P₁) + (1 − s₂)(P₂ − P₁)] (0x66a950, per component).
  At the first and last segment the missing neighbour is clamped to the end key (P-13).
  Integer and text attributes take the value of key *i* (step).
* A looping sequence (period stored with the sequence; set by the game, P-13) wraps *t* modulo
  the period, the last key interpolating towards the first.

### 6.6 Widgets and input

Widgets receive mouse enter/leave/press/release (playing the button sequences above), and the
`Edit` widget takes typed characters (the game forwards WM_CHAR to the interface, 0x652710,
and key down/up to 0x652730 / 0x652790). The game attaches callbacks to buttons by name
(`alpha-ui-screens.md`).

---

## 7. The fonts

`resource1.dat` and `resource2.dat` are **ordinary TrueType fonts** with a changed extension
(table directory starts 00 01 00 00; built with FontForge, `FFTM` table). They are loaded from
the install folder by the embedded FreeType library (the executable contains FreeType's TrueType,
Type 1, CFF, PFR, BDF and Windows-FNT drivers; the folder `c:\windows\fonts` is also searched,
0x4c8807).

| File | Family / style | Units per em | Ascender / descender / gap | Glyphs | Coverage | Used for |
|---|---|---:|---|---:|---|---|
| `resource1.dat` | "resource1" Bold | 1000 | 1125 / −250 / 33 (OS/2 win 1125 / 250) | 232 | U+0020–007E, U+00A0–00FF (not U+00AD), Œ œ Š š Ÿ Ž ž ƒ ı Ł ł, typographic quotes, dashes, bullet, ellipsis, per-mille, guillemets; has `kern` | every interface text |
| `resource2.dat` | "resource2" Medium | 1900 | 1500 / −400 / 171 | 151 | U+0020–003F, U+0041–007E (no `@`), selected Latin-1 letters, Œ œ Ÿ, dashes, quotes | land-name banner (`landname` → `name`, `detail`) |

Render text with FreeType (or an equivalent TrueType rasteriser) at the pixel size of §4.4,
grey-scale anti-aliased. Text drawn by the game directly (not from a .plx) — start menu, system
menu, options, labels created in code — uses `resource1.dat` with sizes given in
`alpha-ui-screens.md`.

---

## 8. Checking

1. **Parsing**: all five files must parse with every record ending exactly where its size
   says; the name table of `gui.plx` has 115 names, the others 69–88; counts as in §3.8.
2. **Seal**: unscrambling the 14 seal bytes 78 87 81 6D BD 3D 2F C7 6E 99 AC 39 28 D1 with the key
   of §2.2 gives `PlasmaGraphics`; the first scrambled name in `gui.plx` (80 98 9F B0 32 3F D2)
   gives `Texture`, in the other files (15 bytes) `SmoothMeshShape`.
3. **Textures**: the four star textures inflate to exactly width × height × 4 bytes.
4. **Visual**: frames captured with the user's Alpha mod DLLs (start screen, HUD, inventory,
   quest banner) compared with AlphaVulkan's rendering of the same nodes at the same window
   size.

---

## 9. Open questions (and how to settle them)

* **P-1** Initial current value. Settle by reading attribute +0x20 ("current frame index") of a
  freshly loaded Display (G+0x800884 tree) with a mod DLL, or by checking whether `questtag`
  is drawn 130 or 110 high (its frame 0 and frame 1 differ).
* **P-2** Texture byte order (RGBA vs BGRA): capture the item star rating (`star1`–`star4`).
* **P-3** `vertexFlags` high bits (0x10003) and `vertexParameters`: trace 0x63fec0.
* **P-4** Mesh `flags` bit 0, stroke alignment sign, joint/cap types: trace the stroke builder
  in 0x644fa0 / 0x642ad0; or capture `graywidget` (stroke width 3, alignment 1).
* **P-5** `Display.flags` (2, 4) and which mask filter they select: trace the drawing of a node
  with clipping (0x632870 frame draw, D3D9Drawing at 0x689f30) and the `filter` constant set.
* **P-6** `Node.flags` 0x800 / 0x80 / 0x2000 meanings (layer/caching/group). Trace 0x64f4e0 and
  the draw walk.
* **P-7** Transform order and rotation sense (see §4.1).
* **P-8** Interior subdivision of multi-face meshes and the fill of overlapping faces (0x644fa0,
  ~15 KB). Compare a capture of `start.plx`'s letters (subdivided) with a render.
* **P-9** TextShape flag bits, line height and outline style: trace 0x685b10's consumers
  (FontEngine at 0x64d0f0) or capture labels.
* **P-10** Colour inheritance down the tree (multiplicative assumed).
* **P-11** Which widget rectangle drives the 9-slice.
* **P-12** Whether clones share shapes.
* **P-13** Clamping of missing neighbour keys and the loop period source (0x662300's caller).
