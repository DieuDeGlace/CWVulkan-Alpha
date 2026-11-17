# Alpha interface: the Plasma `.plx` files, their runtime and the fonts

Status: FINAL / STABLE / HELD (re-certified 2026-11-16; see "Re-certification (2026-11-16)" at the end)

Behavioural specification of the interface library ("Plasma", by Picroma) built into the Cube
World **Alpha** client (`Cube.exe`, SHA-256 `84a7a132…30bf`), and of the five interface files it
loads from the install folder: `gui.plx`, `start.plx`, `help.plx`, `cursor.plx`, `quest-tag.plx`,
plus the two fonts `resource1.dat` and `resource2.dat`. Written from the Alpha decompile
(`disassembled alpha cube world/decompiled/client`), its assembly and the bytecode of the
interface shaders in the executable, and **checked against the files themselves**: every
statement in sections 2–5 marked *(verified)* was confirmed by parsing all five files from the
first to the last byte.

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
* Matrices are 4×4, stored row-major, and act on **row vectors** (x, y, z, 1) multiplied from
  the left; the translation is in the last row.

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
index. The game then finds subtrees **by node name** and clones them (§6.3, §6.4).

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
property). Tokens are global for the file; the file must therefore be walked in order,
descending into nested payloads in order. (Reader: 0x688220; plain name read 0x688510.)

Every payload is self-delimiting by its size, so any record can be skipped without being
understood (the game does: unknown top-level records and unknown properties are skipped,
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
  ("invalid demo licence"). In all five files the passing key is the one above.

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
or a wstring (text). An `ArrayAttribute.frame` payload is *m* consecutive values. Frames are
stored into the attribute's frame slots 0, 1, 2, … in file order (0x680b40 and its siblings);
every attribute is constructed with two frame slots (0x678080 → 0x66b180) and grows as frames
are read.

### 2.4 Frames and sequences (animation data)

Every animatable property stores **frames**: frame 0, frame 1, and possibly more.

* **Frame 0** is the authored value **and the current value**. Each attribute holds the index
  of its current-value frame (attribute +0x20); it is set to 0 when the attribute is
  constructed (0x661480) and the file loader never changes it (0x680b40). Drawing reads the
  frame at that index (e.g. the node matrix, 0x678600), game writes go there, and sequence
  evaluation writes its result there (0x66a950, 0x6696d0, 0x6531a0). An evaluated or
  game-written value therefore replaces the authored frame-0 value and stays until changed
  again.
* **Frame 1** is a scratch slot. Its contents in the files (editor state or left-over memory;
  matrices often contain garbage) are never displayed. When a sequence starts whose first key
  is not at time 0, the current value is copied into frame 1 (0x66b0c0, called from 0x661df0)
  and used as an implicit starting key (§6.5).
* **Frames 2…** are keyframe values used by sequences (every key in the shipped files refers
  to frame 2 or higher).

An `Attribute.sequence` record contains:

* `Attribute.sequence.wname` (wstring): the sequence name, e.g. `shine`, `show`, `show2`, `hit`,
  `manacubes`, `button:enter`, `button:leave`.
* one `Attribute.sequence.key` record per key, in time order, each containing
  `Attribute.sequence.key.frame` (i32: which frame of this attribute holds the key value),
  `Attribute.sequence.key.time` (i32 milliseconds from the start of the sequence) and
  `Attribute.sequence.key.smoothness` (f32, 0..1; seen values 0 and 1).

A sequence may have no keys (many in `gui.plx` and `quest-tag.plx` do). A sequence with the
same name may exist on many attributes of many objects; playing a sequence on a node subtree
plays every attribute sequence of that name in it (§6.5).

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
the file is fixed, but the loader matches properties by name and skips unknown ones.

Page values in the files: `gui.plx` 760 × 530, `start.plx` 1600 × 1000 (dpi 235.93), the others
1000 × 1000; dpi 72 in all but `start.plx`. Page colours: `gui.plx` (0.21346, 0.703448, 0, 1),
`start.plx` (0.551724, 1, 0, 1), `help.plx` (0.227586, 0.568514, 1, 1), `quest-tag.plx` and
`cursor.plx` (1, 1, 1, 0). The page is editor information only; the game ignores it.

---

## 3. The objects (verified field lists)

Field types: see §2.3. "attr" = attribute with frames; "array attr" = array attribute. Counts
are over all five files.

### 3.1 `Texture`

| Field | Type | Notes |
|---|---|---|
| `wname` | wstring | `star1.png` … `star4.png` (the only four textures, all in `gui.plx`) |
| `id` | int | the number shapes use to refer to it (2, 4, 5, 3 for star1, star2, star3, star4; file order star1, star3, star2, star4) |
| `format.pixelFormat` | int | 1 in all files: 8-bit RGBA |
| `format.minFilter`, `format.maxFilter` | int | 0 |
| `format.horizontalWrap`, `format.verticalWrap` | int | 0 |
| `width`, `height` | int | 50 × 50 (star4 52 × 50) |
| `compressedPixels` | u32 size *s*, then *s* bytes of a **zlib** stream (header 78 9C) | inflates to exactly width × height × 4 bytes, rows top to bottom (0x686820 → 0x449540) |
| `pixels` | uncompressed variant (not used in the files) | |

Byte order of a pixel: believed R, G, B, A. Read that way, the dominant opaque colours of
star1–star4 are green (51 FF 51), blue (64 7C FF), purple (B1 62 FF) and orange (FD 93 00);
the transparent border pixels are 00 00 00 00. The upload path was not traced (open question
P-2).

### 3.2 `SmoothMeshShape` (321 shapes)

A **polygon mesh** in the plane: shared vertices, polygon faces (mostly quads, also long
outline polygons), per-vertex colours, optional stroke along the outline, optional subdivision.

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | shape name (editor name; many are `procedural:preview`, meaningless) |
| `face` | raw int array, **repeated once per face** | the face's vertex indices in order |
| `vertexFlags` | raw int array (one per vertex) | corner type, low 16 bits 0–3 (§4.2); 0x10003 also occurs (136 vertices, in `gui.plx` and `help.plx`; high bits: open question P-3) |
| `vertexParameters` | raw int array (one per vertex) | mostly 0; otherwise f32 bit patterns near 5, 8, 10, 15 (meaning unknown, P-3) |
| `vertexPositions` | array attr of vector2 | |
| `vertexTexCoords` | array attr of vector2 | 0..1 |
| `vertexColors` | array attr of colour | fill colour per vertex |
| `strokeColors` | array attr of colour | stroke colour per vertex |
| `extrusionFrontColors`, `extrusionBackColors` | array attr of colour | used only with flag 16 |
| `strokeWidths` | array attr of f32 | stroke width per vertex (page units) |
| `texture`, `strokeTexture` | attr of int | `Texture.id`, or −1 for none (only the four star shapes use a texture; `strokeTexture` is −1 everywhere) |
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
| `strokeDash`, `strokeGap` | float | dash 1 everywhere; gap 1 or 2 (unused with pattern 0) |
| `smoothWeight` | float | 0.585786 = 2 − √2 everywhere (the library default, 0x63c2b0) |

### 3.3 `TextShape` (194 shapes)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | node-ish name (`text`, `info`, `name`, `count`, `class`, `objective`, …) |
| `string` | attr of wstring | the text; the game replaces it at run time |
| `color` | attr colour | fill colour of the glyphs |
| `strokeColor` | attr colour | outline colour |
| `extrusionColor` | attr colour | mostly 0.5 grey (also black, white and other values in `help.plx`); no visible use known |
| `flags` | int | layout flags, §4.4 (0, 1, 2, 4, 5, 6, 17, 21 seen) |
| `wfontName` | wstring | `resource1.dat` (190 shapes) or `resource2.dat` (4: the land name banner) |
| `pixelSize` | float | glyph size in page units: 8, 9, 10, 12, 14, 16, 20, 22, 40, 69 (some stored with rounding noise, e.g. 9.999998) |
| `strokeRadius` | float | outline radius in page units (0 = no outline; 2–7 seen) |
| `spacing` | float | extra letter spacing (0 everywhere) |
| `lineSpacing` | float | extra line spacing (0, or 2) |
| `wrapWidth` | float | wrapping width (200 in `gui.plx`, `quest-tag.plx`, `cursor.plx` by default; 258 for quest objectives, 400 for land names; 10 in every `help.plx` text, none of which has the wrap flag) |

Most labels exist as **two text nodes with the same name at the same place**: the first has an
outline (`strokeRadius` > 0, often black or dark), the second none. The game writes the same
text into both. Drawn in order, the second covers the inside of the first.

### 3.4 `Transformation` (601 objects)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring (empty) | |
| `translation` | attr vector2 | offset of the node in its parent's space (§4.1) |
| `rotation` | attr vector3 | degrees about x, y, z; only z is non-zero in the files (90 and 270, one node each) |
| `pivot` | attr vector2 | centre of rotation and deformation (§4.1) |
| `deformation` | attr matrix | full 4×4 matrix (scale/shear, and its translation row), row-major, identity in most |

### 3.5 `Display` (601 objects)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring (empty) | |
| `visibility` | attr int | 1 shown, 0 hidden (hides the whole subtree). The game opens and closes windows by toggling this value (0x488bd0, 0x488c00, 0x488d00) |
| `clipping` | attr int | 1 = this node's shape is a **mask** for its children (§5.3); 10 nodes use it (5 in `gui.plx`, 1 in `start.plx`, 4 in `quest-tag.plx`) |
| `fillColor` | attr colour | multiplies the colours of the node and its subtree (§5.1) |
| `strokeColor` | attr colour | multiplies stroke colours |
| `blurRadius` | attr f32 | 0 everywhere (blur pass §5.4) |
| `flags` | int | 0; 2 on 7 mask nodes (5 in `gui.plx`, 2 in `quest-tag.plx`); 4 on the one mask node in `start.plx`; the other 2 mask nodes of `quest-tag.plx` have 0 (P-5) |

### 3.6 `Widget`, `Button`, `Edit` (37 objects in `gui.plx`)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | e.g. `lifebar`, `button`, `edit`, `itembox` (the one `Edit` is named `text`) |
| `caption` | wstring (empty) | |
| `innerBindPos`, `innerBindSize` | vector2 | the stretchable inner rectangle (§5.5) |
| `bindPos`, `bindSize` | vector2 | the widget's rectangle as drawn in the file |
| `framePos`, `frameSize` | vector2 | equals bind rectangle in the files |
| `bindMatrix` | 16 f32 | the widget's placement matrix at bind time |
| `horizontalAlignment`, `verticalAlignment` | int | 0, or 2 (3 widgets) |
| `flags` | int | 0, or 4 (2 widgets) |
| `Button.type` | int (buttons only) | 0 |

(These fields are plain values, not attributes.)

### 3.7 `Node` (618 objects)

| Field | Type | Meaning |
|---|---|---|
| `wname` | wstring | the name the game searches for |
| `shape` | int | index into the shape list, −1 = none |
| `transformation` | int | index into the transformation list |
| `display` | int | index into the display list |
| `widget` | int | index into the widget list, −1 = none |
| `child` | int, **repeated once per child** | child node indices, in drawing order |
| `flags` | int | 0x2000 on node 0 of every file; 0x800 on `combopoints` (`gui.plx`), `questtag` and `smallquesttag`; 0x880 on `help` and on 14 groups of `start.plx` (including `cubeworld` and `picroma`) (P-6) |
| `Node.variable` | two strings | not present in the files |

**Node 0 of every file is the document root and is not created**: the loader attaches node 0's
children to the container it is loading into, and node 0's own shape, transformation and
display are ignored (0x683f00 with the "first node" flag). Its transformation in the files is
the editor's view (uniform scale 2.25 in `gui.plx` and `help.plx`, 1.70325 in `start.plx`,
1.1355 in `quest-tag.plx`, 3.832313 in `cursor.plx`, plus an editor pan) and is not applied.

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
| Names defined | 115 | 69 | 82 | 88 | 82 |

`start.plx` is large because its letters (C, U, B, E, W, O, R, L, D, "Picroma", "©2013") are
dense meshes with up to 228 faces. Only `quest-tag.plx` and `gui.plx` contain sequences:

* `quest-tag.plx`: `shine`, `show` and `show2` on many attributes of `questtag` and
  `smallquesttag` (vertex and stroke colours, text stroke colours, translations, rotations,
  visibility).
* `gui.plx`: `hit` (vertex colours of `combopoints` itself, keys at 0/400/933 ms, and the
  translation of its `Text` child, keys at 67/867 ms) and `button:enter` (that same `Text`
  translation); `manacubes` on the vertex colours of `manacubebar` itself (keys at 0, 600 and
  1300 ms, smoothness 1); `button:enter` / `button:leave` on the templates `button`,
  `button2` (their own shape, their `clip` child and a `Shape` translation below it) and
  `smallbutton` (its shape's fill and stroke colours and both `text` children's colours).

---

## 4. Geometry

### 4.1 Transform of a node

A node's local matrix is built by 0x678600 from its translation **t**, pivot **p**, rotation
angles (θx, θy, θz) in degrees (multiplied by 0.0174533) and deformation matrix **D**. In the
row-vector convention of §0 the matrix is

**M = T(−p) · D · Rz(θz) · Ry(θy) · Rx(θx) · T(p) · T(t)**

so a point **x** of the node's shape goes through, in this order: subtract the pivot; apply D
(the full 4×4 matrix, including its translation row); rotate about z, then y, then x; add the
pivot; add the translation. In short **x′ = t + p + R·D·(x − p)**. For identity R and D a shape
point simply moves by **t** (x′ = x + t); the pivot is the fixed point of rotation and
deformation, not an offset.

* Rz(θ) maps (x, y) to (x·cos θ − y·sin θ, x·sin θ + y·cos θ). With the y axis pointing down,
  a positive angle turns +x towards +y, i.e. **clockwise on screen**. Thus `upbutton` and
  `downbutton` (the `leftbutton` arrow rotated by 90° and 270°) point up and down.
  Rx(θ) maps (y, z) to (y·cos θ − z·sin θ, y·sin θ + z·cos θ); Ry(θ) maps (x, z) to
  (x·cos θ + z·sin θ, −x·sin θ + z·cos θ).
* After the product, the z column of M is forced to (0, 0, 1, 0) (0x678600 end).
* A node's full matrix is its local matrix followed by its parent's full matrix,
  **W = M · W_parent** (0x630be0 when a child is attached, 0x6371b0 on update); a node with no
  parent uses M itself.

The game anchors a node by writing translation = anchor − pivot, so that the pivot lands on the
anchor (`alpha-ui-screens.md` §2.1).

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
linearly across it (Gouraud). Outline polygons such as `frame`, `graywidget` and the letters
are non-convex and are filled as true polygons (the library triangulates through its
quad-edge structure, not traced). Shapes with flag 4 have no fill. Faces are believed to be
drawn independently in face order (P-8).

**Stroke** (flag 2). A band of width `strokeWidths` (interpolated along the outline) around the
**boundary edges** of the mesh (edges used by only one face), coloured with `strokeColors`,
round joins and caps for type 1. Alignment 0 centres the band on the outline; 1 and −1 put it
inside or outside (P-4).

**Texture** (only `star1`–`star4`): texture coordinates from `vertexTexCoords`, transformed by
the texture translation/rotation/deformation/pivot into the shader's texture matrix (applied
as (u, v, 1) against its x, y and w columns, §5.6); sampled colour combined as in §5.2.

### 4.3 Anti-aliased edges (widget vertex shader 0x720558, verified from bytecode)

The library adds a thin fringe to outlines instead of multisampling. Fringe vertices carry two
edge normals (texture-coordinate inputs 0 and 1). Only a vertex whose two input normals are
both non-zero, and whose two normals are still non-zero after the 2-row `NormalMatrix`, is
moved; any other vertex is left in place. For a moved vertex:

* each transformed normal is normalised (length + 0.00001 in the divisor);
* the bisector = normalised sum of the two (same guard);
* the push distance = `aaOffset` ÷ max(cos, 0.7), where cos = dot(first normal, bisector);
* the view-space position moves by bisector × distance in x and y and by **+0.5 in z**; the
  position used for the mask coordinate (§5.3) moves by the same x/y offset.

The visual result is soft edges about one pixel wide.

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
  The library has a "glow" outline font path (`TextShape.font.glowRadius`, `strokeGlow`); the
  outline is believed to be a soft (glow) outline of that radius (P-9).
* **Colour**: `color` × display fill colour (§5.1).

---

## 5. Drawing

### 5.1 Order and colour inheritance

The tree is drawn depth-first: a node's own shape, then its children in `child` order (later
children on top). A hidden node (visibility 0) hides its subtree. The display `fillColor`
multiplies the vertex/glyph colours of its node and (inherited multiplicatively) of all
descendants; `strokeColor` does the same for strokes (P-10 for the exact inheritance). All
interface drawing is alpha-blended over the 3D frame, after it (`alpha-rendering.md` §1.3
step 19, "later world-space and interface drawing"). Whether depth testing is off for the
interface was not re-traced (the vertex shader still writes a depth, §4.3) (P-14).

### 5.2 The widget pixel shader (0x720e28, verified from bytecode)

Constants: `textureEnabled` (bool b0), `filter` *F* (c0), `textureOpacity` *o* (c1),
`textureBrightness` *b* (c2), `textureContrast` *k* (c3), `textureSaturation` *s* (c4);
samplers `maskSampler` (s0) and `textureSampler` (s1); inputs: vertex colour **v**, mask
coordinate, texture coordinate.

1. Without texture: colour = **v** × *k* (the contrast constant, normally 1; all four channels).
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

The plain textured pixel shader (0x721420) un-premultiplies the texture (when alpha > 0) and
multiplies by the vertex colour: off-screen layers are stored premultiplied.

### 5.3 Masks (clipping nodes)

A node whose display has `clipping` = 1 is a mask: its own shape is rendered into a mask layer
(not shown), and its descendants are drawn through it with the mask filter of §5.2 — which
filter value corresponds to `Display.flags` 0, 2 and 4 is open (P-5; the natural reading is
flags 2 → alpha mask, so that e.g. the quest banner's `shine` highlight and the land-name
banner are cut to the banner's shape). The mask coordinate is the descendant's position (in
its node space, after widget stretching and the edge fringe) transformed by the 2-row
`MaskMatrix` (vertex shader 0x720558).

### 5.4 Blur layers

A display with `blurRadius` *r* > 0 is drawn into an off-screen layer, which is blurred with a
separable 9-tap filter: weights 0.18 at the centre and 0.15, 0.12, 0.09, 0.05 at offsets ±1…±4
× `blurScale` (sum 1.0; x pass 0x722270, y pass 0x721f00). `blurScale` = 0.25 × *r* ÷ the
layer **width**, for both passes (0x68cad0 divides by the width in either branch). The layer
may first be box down-sampled 2×2, 3×3 or 4×4 (0x721530, 0x721778, 0x721a98; each averages
its taps around the cell picked by `textureScale`, un-premultiplies when alpha > 0 and
multiplies by the vertex colour). No node in the shipped files has *r* > 0.

### 5.5 Widget stretching (9-slice) (verified from bytecode)

When the game resizes a widget node to a new size (e.g. buttons to 300 × 40, windows to their
panel sizes), its geometry is deformed by the widget vertex shader (0x720558,
`widgetDeformationEnabled`) rather than scaled. With the bind rectangle (position **B**, size
**S**) and the deformed rectangle (position **B′**, size **S′**), each vertex is first taken
through `widgetBindMatrix` to **p** (bind space) and moved to

**p′ = p + (B′ − B) + u ⊙ (S′ − S)**, with uᵢ = 0 if pᵢ < Bᵢ, 1 if pᵢ ≥ Bᵢ + Sᵢ, else
(pᵢ − Bᵢ)/Sᵢ

per axis (x and y only; z and w are kept), then taken back through
`inverseWidgetBindMatrix`. Geometry left of / above the rectangle keeps its offset, geometry
right of / below it moves with the far edge, and geometry inside is stretched — a nine-slice.
Which rectangle is passed as **B**/**S** (the widget's `innerBindPos/Size` or `bindPos/Size`)
is open (P-11); the inner rectangle gives the expected look (corners keep their shape).

### 5.6 Interface shaders (for reference)

Created by 0x68a350: widget vertex shader 0x720558 (vs_3_0: `Proj` c0, `WorldView` c4,
`widgetBindMatrix` c8, `inverseWidgetBindMatrix` c12, `MaskMatrix` c16, `TextureMatrix` c18,
`NormalMatrix` c20, `widgetBindPos` c22, `widgetBindSize` c23, `deformedWidgetPos` c24,
`deformedWidgetSize` c25, `aaOffset` c26, `baseColor` c27, `widgetDeformationEnabled` b0);
widget pixel shader 0x720e28 (ps_3_0); pass-through vertex shader 0x7212c0; plain textured
pixel shader 0x721420; box down-samplers 0x721530 (2×2), 0x721778 (3×3), 0x721a98 (4×4);
blur x 0x722270 (ps_3_0) and y 0x721f00 (ps_2_0). In the widget vertex shader the vertex
colour is multiplied by `baseColor`; after `WorldView` and the edge fringe, the view-space
position gets a fixed −0.135 offset in x and y (Direct3D 9 pixel-centre alignment) before
`Proj`; the texture coordinate is (u, v, 1) dotted with the x, y and w columns of the two
`TextureMatrix` rows.

---

## 6. Runtime behaviour the game relies on

### 6.1 Loading

`load(file, container, flags)` (0x653770, flags 0x20 from the game): parse §2, build every
object, link nodes, attach node 0's children to *container*. Loading never touches the page
records.

### 6.2 Current values

Every attribute has a current value used for drawing: its frame 0 (§2.4). The game changes
interface state by writing current values: visibility (windows), translation (anchoring,
bar positions), deformation (bar fill, `alpha-ui-screens.md` §2.1), text strings, colours,
vertex positions.

### 6.3 Node lookup

`find(container, name)` (0x633d70 / 0x636040) returns the first node with that `wname` in a
depth-first search. Names are not unique (`text`, `info`, `bar` occur in every bar); the game
looks them up **within** a cloned template.

### 6.4 Cloning

`clone(node)` (0x6326d0) deep-copies a subtree (nodes, transformations, displays, and per the
use of shared shapes, shapes by reference unless the game edits them, P-12). Every HUD element
is a clone of a `gui.plx` template.

### 6.5 Playing a sequence

**Starting.** `play(node, name, period)` (0x636810) starts the named sequence on the node's
shape, transformation and display, then recursively on every child. For each of these objects,
every attribute that has a sequence of that name **with at least one key** is started
(0x664c10 → 0x661df0), and the object is marked as playing (+0x2c). A node counts as
"playing" while its own shape, transformation or display is, or any child is, except children
whose node word +0xC8 has bit 2 set (0x6364f0). Starting an attribute:

1. its working key list is rebuilt from the sequence's keys, sorted by time (0x6779e0); with a
   period ≥ 1 only keys with time < period are kept;
2. if the first key is not at time 0, the current value is copied into frame 1 (0x66b0c0) and a
   key (frame 1, time 0, the first key's smoothness) is inserted first — the sequence starts
   from whatever value the attribute had and moves towards its first key;
3. the elapsed time is reset to 0; the period becomes the sequence's loop period (0 = play
   once).

Calls by the game (all with period 0): `show` on `smallquesttag` (G+0x80086c, 0x48d259);
`manacubes` on G+0x8007d8 (`manacubebar`, 0x48f9e1, on a path taken only when the local
player's mana-cube count +0x1164 is a multiple of 4; what triggers that path was not
traced); `shine` on `questtag` (G+0x800868, 0x490711); `hit` on `combopoints` (G+0x800ad0, 0x493d0a). The `Button` widget constructor (0x664d60) registers the
state sequence names `button:press`, `button:release`, `button:enter`, `button:leave` and their
`:checked` variants. No call playing `button:enter` on `combopoints` was found.

**Evaluating** (0x662300), at time *t* milliseconds after the start, on the working key list
of *n* keys:

* *n* = 0: nothing; *n* = 1: current = that key's frame;
* *t* before the first key: current = first key's frame;
* *t* at or after the last key and period 0: current = last key's frame;
* otherwise find the segment of keys *i*, *i*+1 with tᵢ ≤ *t* ≤ tᵢ₊₁ (the search starts at the
  segment found last time), u = (*t* − tᵢ)/(tᵢ₊₁ − tᵢ), and with the neighbouring key values
  P₀ (key *i*−1), P₁ (key *i*), P₂ (key *i*+1), P₃ (key *i*+2) and the smoothness values
  s₁ (key *i*) and s₂ (key *i*+1), evaluate the cubic Bézier
  **value = (1−u)³·P₁ + 3u(1−u)²·C₁ + 3u²(1−u)·C₂ + u³·P₂** with
  C₁ = P₁ + ¼[s₁(P₂ − P₀) + (1 − s₁)(P₂ − P₁)] and
  C₂ = P₂ − ¼[s₂(P₃ − P₁) + (1 − s₂)(P₂ − P₁)] (0x66a950 for vectors, per component).
  With period 0, a neighbour index below 0 or above *n* − 1 is clamped to the first or last
  key (0x630970).
  Integer and text attributes step: the value of key *i* while u < 1, of key *i*+1 at u = 1
  (0x6531a0 for integers).
* With a period *T* ≥ 1 (looping): the key times are shifted by ⌊*t*/*T*⌋·*T* (integer
  division), there are *n* segments, the last one runs from key *n* − 1 to key 0 one period
  later, and neighbour indices wrap modulo *n*.
* A segment is skipped (no change this time) if a key refers to a frame that does not exist.

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
| `resource1.dat` | "resource1" Bold (weight 700) | 1000 | 1125 / −250 / 33 (OS/2 win 1125 / 250) | 232 | U+0020–007E, U+00A0–00FF (not U+00AD), Œ œ Š š Ÿ Ž ž ƒ ı Ł ł, typographic quotes, dashes, bullet, ellipsis, per-mille, guillemets; has `kern` | every interface text |
| `resource2.dat` | "resource2" Medium (weight 500) | 1900 | 1500 / −400 / 171 | 151 | U+0020–003F, U+0041–007E (no `@`), selected Latin-1 letters, Œ œ Ÿ, dashes, quotes | land-name banner (`landname` → `name`, `detail`) |

Glyphs are rasterised by FreeType at the pixel size of §4.4 (believed grey-scale
anti-aliased; the render mode was not re-traced). Text drawn by the game directly (not from a
.plx) — start menu, system menu, options, labels created in code — uses `resource1.dat` with
sizes given in `alpha-ui-screens.md`.

---

## 8. Evidence checks

1. **Parsing**: all five files parse with every record ending exactly where its size says;
   the name counts and object counts are those of §3.8.
2. **Seal**: unscrambling the 14 seal bytes 78 87 81 6D BD 3D 2F C7 6E 99 AC 39 28 D1 with the key
   of §2.2 gives `PlasmaGraphics`; the first scrambled name in `gui.plx` (80 98 9F B0 32 3F D2)
   gives `Texture`, in the other files (15 bytes) `SmoothMeshShape`.
3. **Textures**: the four star textures inflate to exactly width × height × 4 bytes.

---

## 9. Open questions

* **P-2** Texture byte order (RGBA vs BGRA): the upload from `compressedPixels` (0x686820 →
  0x449540) was not traced; the colours of §3.1 favour RGBA.
* **P-3** `vertexFlags` high bits (0x10003) and `vertexParameters`: 0x63fec0 not traced.
* **P-4** Mesh `flags` bit 0, stroke alignment sign, joint/cap types: the stroke builder in
  0x644fa0 / 0x642ad0 was not traced.
* **P-5** `Display.flags` (0, 2, 4 on mask nodes) and which mask filter they select: the
  drawing of a node with clipping (D3D9Drawing at 0x689f30) and the `filter` constant set were
  not traced.
* **P-6** `Node.flags` 0x800 / 0x80 / 0x2000 meanings (layer/caching/group); 0x64f4e0 and the
  draw walk not traced.
* **P-8** Interior subdivision of multi-face meshes and the fill of overlapping faces (0x644fa0,
  ~15 KB) not traced.
* **P-9** TextShape flag bits, line height and outline style: 0x685b10's consumers (FontEngine
  at 0x64d0f0) not traced.
* **P-10** Colour inheritance down the tree (multiplicative assumed).
* **P-11** Which widget rectangle drives the 9-slice.
* **P-12** Whether clones share shapes.
* **P-14** Depth-test state during interface drawing.

(P-1, P-7 and P-13 are resolved: §2.4, §4.1, §6.5.)

---

## Re-certification (2026-11-16)

Re-checked against `Cube.exe` (SHA-256 84A7A132…A6DC30BF, matching the stated image), its
decompile and assembly, a byte-level parse of all five shipped `.plx` files, a disassembly of
the shader bytecode at 0x720558, 0x720e28, 0x721420, 0x721530 and 0x722270, and the font
headers. Only the held specs `alpha-ui-screens.md` (§2, §5.3) and `alpha-rendering.md` (§1.2,
§1.3) were consulted for cross-reference.

### Corrections

1. §4.1 node transform (the pivot term flagged by the screens author). Old:
   x′ = t + R·D·(x − p), "translation = target point", pivot as hot spot, order of R and D and
   rotation sense untraced (P-7). New: M = T(−p)·D·Rz·Ry·Rx·T(p)·T(t), i.e.
   x′ = t + p + R·D·(x − p); D is applied before the rotations and in full (4×4); rotation
   order z, y, x on the point; positive z angle is clockwise on screen; z column forced to
   (0, 0, 1, 0). Evidence: 0x678600 (translation +0x48, pivot +0x150, rotation +0xf8,
   deformation +0xa0; constants 0.0174533 at 0x6fcda4 and −1 at 0x745e8c). P-7 resolved.
2. §4.1 hierarchy. Old: "product of its ancestors' transforms (root first)". New: W = M ·
   W_parent (own matrix first, then the parent's). Evidence: 0x630be0, 0x6371b0.
3. §2.4 / §6.2 current value (P-1). Old: frame 1 is the current-value slot written by the
   evaluator; initialise from frame 0 as an open question. New: the current-value index is 0
   from construction and never changed by the loader, so frame 0 is the current value and is
   overwritten by evaluation and game writes; frame 1 is the start-of-sequence snapshot.
   Evidence: 0x661480 (+0x20 = 0), 0x680b40 (frame reader), 0x66a950 / 0x6696d0 / 0x6531a0
   (write to the current index), 0x66b0c0 + 0x661df0 (snapshot into frame 1). P-1 resolved.
4. §6.5 sequence start (omitted). New: keys sorted, period filter, implicit (frame 1, time 0)
   start key when the first key is later than 0, playing flag. Evidence: 0x636810, 0x664c10,
   0x661df0, 0x6779e0, 0x6364f0.
5. §6.5 loop period and neighbour clamping (P-13). New: the period is the play call's third
   argument (0 for every game call); clamping for period 0, wrapping and per-period time shift
   for looping; segment search starts at the last segment. Evidence: 0x661df0 (period stored
   from the argument), 0x662300, 0x630970. P-13 resolved.
6. §6.5 game calls. Old: `hit` and `button:enter` on `combopoints`; button states played by
   0x664d60. New: `show` on `smallquesttag` (0x48d259), `manacubes` on G+0x8007d8 (0x48f9e1),
   `shine` on `questtag` (0x490711), `hit` on `combopoints` (0x493d0a); 0x664d60 is the Button
   constructor that registers the state names; no call plays `button:enter` on `combopoints`.
7. §3.8 sequences. Old: only `quest-tag.plx` and `gui.plx` (`combopoints`) contain sequences.
   New: `gui.plx` also has `manacubes` on `manacubebar` and `button:enter`/`button:leave` on
   `button`, `button2`, `smallbutton`. Evidence: file parse.
8. §3.4 count. Old: 626 Transformation objects. New: 601 (338 + 66 + 168 + 25 + 4).
9. §3.7 count. Old: 623 Node objects. New: 618 (355 + 66 + 168 + 25 + 4).
10. §2.5 page values. Old: the others 1000 × 1000, dpi 72, page colour (1, 1, 1, 0). New:
    per-file page colours; dpi 72 also in `gui.plx`; only `quest-tag.plx` and `cursor.plx` use
    (1, 1, 1, 0). Evidence: file parse.
11. §3.3 `wrapWidth`. Added: every `help.plx` text has 10 (no wrap flag). `extrusionColor`
    "unused (0.5 grey)" changed to "mostly 0.5 grey; no visible use known".
12. §3.5 `Display.flags`. Added: 2 of the 4 `quest-tag.plx` mask nodes have flags 0; per-file
    clipping counts.
13. §3.7 `Node.flags` and node 0. Exact flag placement (0x800 on 3 groups, 0x880 on `help`
    and 14 `start.plx` groups); editor-view scales of every file.
14. §3.1 star pixel sample. Old: star4 reads FD 82 00 FF. New: dominant colours of all four
    stars; zlib header and inflated sizes stated.
15. §4.3 edge fringe. Added: only vertices with two non-zero normals move; NormalMatrix,
    1e-5 guards, cosine floor 0.7, +0.5 in view z, mask coordinate shifted. Evidence: 0x720558
    bytecode.
16. §5.4 blur scale. Old: 0.25 × r / layer size. New: divided by the layer width in both
    passes; down-sampler behaviour added. Evidence: 0x68cad0, 0x721530.
17. §5.1 citation. Old: `alpha-rendering.md` §1.3 step 15 (the objects pass). New: step 19.
    "Without depth testing" downgraded to open (P-14).
18. §5.2 / §5.5 / §5.6 register assignments, bind-space detail and the texture-coordinate
    form added from the bytecode; the shader formulas of §5.2 and §5.5 were re-derived and
    confirmed unchanged.
19. Removed implementer advice and engine mentions: "an implementation can simply use that
    key", "do not use frame 1 from the file", triangulation and MSAA alternatives, "can be
    implemented last", "omit in Vulkan", "render text with FreeType or equivalent", the
    capture/rendering comparison check and capture/mod-DLL instructions in §9.

### Confirmed unchanged

File sizes; record/name-definition format; header bytes; scrambling key 0xFB3A260B5EC7CD5C
and the seal/first-name examples; object counts per file except corrections 8–9; name counts
115/69/82/88/82; texture ids, sizes and inflate lengths; all listed value sets of shape, text,
widget and display fields; subdivision count 202 at 0; the widget pixel shader formulas;
9-slice formula; blur weights; font metrics, glyph counts and `kern` presence.

### Cross-specification disagreements (body states the facts above; other specs not edited)

* `alpha-ui-screens.md` §5.3, `manacubebar` row: "the file has no sequence there, so
  effectively mod 4". This document: `gui.plx` has a `manacubes` sequence (3 keys, 0/600/1300 ms)
  on the vertex colours of the `manacubebar` node's own shape, and the game plays it
  (0x48f9e1); 0x6364f0 counts a node's own shape, so "a sequence plays in it" can be true
  while it runs. Evidence: file parse; 0x48f9e1; 0x6364f0.
* `alpha-ui-screens.md` §2.1 recorded that this document's node matrix omitted the "+ pivot"
  term. Now agreed (correction 1). Its wording "rotate about x, then y, then z … apply D,
  translate by −p" lists the matrix build order; the order applied to a point is the reverse
  (§4.1). Same result, not a conflict.

### Remaining uncertainties

P-2, P-3, P-4, P-5, P-6, P-8, P-9, P-10, P-11, P-12 and P-14 (§9). Also unverified in this
pass: text layout details (§4.4), how elapsed sequence time advances and when the playing
flag clears, the matrix and colour interpolation variants other than vector3 / integer, and
the FreeType render mode.
