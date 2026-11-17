# Alpha stock blueprint placement

**FINAL / STABLE / HELD**

Supported image is original 32-bit Alpha `Cube.exe`, 3,885,568 bytes,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses are RVAs for image base `0x00400000`. Game offsets start at the
client controller. Its world starts at game `+0x2E4`. Actor offsets include
the 16-byte creature prefix. The geometry covers finite stock model data
and representable arithmetic.

The stock activation and local selection ruling is bounded by the writer
audit in section 1. Sections 2 through 5 cover the native conditional
placement path, including the exact offline world consumer. The held
[resolved model list](alpha-blueprint-zone-delivery.md) gives the twelve
stock asset keys, dimensions and voxel counts.

## 1. Activation and selection writers

The game constructor writes zero to the byte at game `+0x800704`
(`0x05A09B`). All six executable-text encodings of that displacement
resolve to this store and five tests at `0x07CCFF`, `0x09797D`,
`0x09B18D`, `0x09B5B1` and `0x0B3338`. Those are the mouse rotation,
request, aim, slot and draw gates.

The outgoing list at game `+0x8006FC` uses two dwords through `+0x800703`.
Construction, append, send cleanup and destruction address the two header
words or separately allocated nodes. The nearby `+0x800710` slot is
initialized as a pointer, and the checked input and widget routes load
that pointer for their own calls. The receive bundle's last header ends
at game `+0x800697`. The embedded-world-relative displacement
`+0x800420` has no executable-text encoding. The checked paths leave the
mode byte at its initialized value. Evidence is `0x05A06D–0x05A0A8`,
`0x067802–0x067842`, `0x069D19–0x069D21`, `0x069EB7–0x069EF5`,
`0x07E9D6`, `0x09C210–0x09C216` and `0x2E5159`.

The Win32 key routes address a 256-entry game array from `+0x19`.
The focused DirectInput poll maps the physical R sample to game `+0x0F`.
These routes update key state. Their stores stay in the respective input
fields. The next frame's saved R sample is written at
`0x09B593–0x09B59C`. Evidence for the input state ranges is
`0x03B4C0–0x03B536`, `0x03B5C0–0x03B5DD`, `0x0C845B–0x0C84CD`
and `0x0C91DA–0x0C92B5`.

The live model number is the signed byte at creature `+0x18D`. Rotation
is the adjacent signed byte at `+0x18C`. Construction and full reset
clear the pair via the creature-state block, which starts at `+0x10`.
The character restore at `0x04BF84–0x04BF93` copies a newly initialized
state block into the actor. Its subsequent scalar save reads write
position, angle and fields including health, class and progression.
The later bulk equipment read starts at `+0x300`. Those traced save
reads leave the initialized placement bytes intact
(`0x03C276`, `0x046697–0x0466A0`, `0x04BF98–0x04C16E`).

State copy at `0x04B22B–0x04B23F` and state construction from another
record at `0x06623A–0x06624E` transfer the two values. Packet 0 and
packet 1 state decoders have separate masked byte fields 31 and 32 at
record `+0x17C/+0x17D` (`0x0CD253–0x0CD27B`,
`0x0CD847–0x0CD879`). Both client receive paths compare the resident
pointer to the local creature and bypass the corresponding state apply
for the local pointer (`0x06BCC5–0x06BCCB`, `0x06C69A–0x06C6A0`).
The login path snapshots the previous local creature state at
`0x070211–0x070221` and applies that snapshot at `0x070333–0x070343`.

Two additional displacement stores at `0x188C75` and `0x18A881` use a
voxel widget as their base. The constructor installs vtable `0x31A664`
at `0x187FE8`. Its original RTTI locator at `0x3480DC` identifies the
voxel widget, and the vtable's `0x188500` entry keeps the widget pointer
as the base of both stores. That routine initializes and copies an item
record at widget `+0x180` through `0x02C5E0` before the stores. The first
store copies the low byte from widget `+0x3D8`. The second follows the
three-byte match at `0x18A850–0x18A861` and copies a byte from the
indexed widget record, alongside the word at widget `+0x190` and the
fixed byte 2 at widget `+0x18C`. These writes belong to the widget's item
record. The recipe preview updates a separate item destination at widget
`+0x16C` (`0x07BBC7–0x07BBF0`, `0x0A1DB8–0x0A1E05`).

The checked stock inputs, transfers and widget paths supply no nonzero
game-mode writer or local model choice for in-world placement. A stock
input or an untraced computed-address transition into the enabled state
remains unresolved.

## 2. Dormant input routes and offline admission

- With a nonzero mode byte, the frame takes the placement arm. R must be
  down in game `+0x0F` and clear in the previous sample at `0x36B138`.
  It appends a default request with kind 7 to the local creature list
  at `+0x130C` (`0x097959–0x0979C7`, `0x059530`, `0x086100`).
  The seven direct calls to that append helper produce kinds 6, 2, 3,
  5, 8, 7 and 1 at `0x0966B3`, `0x0969FA`, `0x097511`, `0x097649`,
  `0x09771E`, `0x0979C2` and `0x0A27EF`, respectively. This census
  bounds the direct calls. An alternate stock kind-7 producer remains
  untraced.
- The Win32 right-button-down event dispatches button index 1. The game
  handler increments the local actor's rotation byte once when the event
  reaches the placement arm and the mode byte is nonzero. Increment wraps
  at eight bits. The mode-zero continuation goes to the ordinary handler
  (`0x0C85C0`, `0x07CCFA–0x07CD16`).
- The mode-gated frame aim arm adds to the three live binary32 components
  at creature `+0x160`. In order the six key fields `+0x0B`, `+0x0C`,
  `+0x0D`, `+0x0E`, `+0x13` and `+0x12` contribute local directions
  minus Y, plus Y, plus X, minus X, minus Z and plus Z. The rotation
  is the camera yaw and the scale is 0.005 blocks per frame millisecond.
  This arm rejoins before updating the saved R sample. The next mode
  gate bypasses ordinary slot handling
  (`0x09B18D–0x09B4C1`, `0x09B593–0x09B5B1`).

The frame's world pieces run at `0x08CF20–0x08CF49`, ahead of the
R request append. A later world piece visits each resident actor's queue
in insertion order, before its living-health admission. Kind 7 selects
`0x2112F7` through the dispatch at `0x210535–0x210546` and table
`0x222C20`. World byte `+0xB4` must be zero. It reads the actor's live
selection byte, sign-extends it, and looks up the signed key in world
`+0x800154` through `0x198840`. A null result advances to the next
queued request. The mode and missing-model exits rejoin at `0x210692`
and `0x211A44`. The offline turn clears the traversed queue at `0x211A91`.
The connected world turn leaves the queue at `0x211A8D` for the sender.

A queued request with a resolved model therefore reaches the native
offline commit arm. The request supplies its kind. The actor supplies
selection, rotation, aim and position at consumption. A stock input
sequence reaching that commit remains unverified.

## 3. Anchor, orientation and voxel intersection

Take each live actor position at `+0x10` as a signed 64-bit fixed value.
Multiply the corresponding aim component by binary32 65,536 and truncate
toward zero to signed 64 bits. Add it to position, then add 32,768 fixed
units. Divide by 65,536 in signed integer arithmetic with truncation
toward zero to get the signed 32-bit anchor block. The next helper
`0x1A5AF0` copies the three anchor coordinates unchanged. The anchor
computation has no terrain or ray query (`0x21132F–0x2113C7`,
`0x02C460`, `0x050F60`).

Start with the identity matrix. Sign-extend the rotation byte and
multiply by binary32 90 degrees. The yaw helper converts to radians
in binary32, evaluates sine and cosine from the promoted value in
binary64, and rounds them to binary32. Positive quarter-turns rotate
model +X toward world +Y. Translate in the rotated frame by half negative
model size X, half negative model size Y and zero Z. The stored transform
sums the rounded X and Y products, adds the Z product, then adds its
translation for each axis. The model is centered horizontally and its
base is at anchor Z. Acquire the world mutation lock at world
`+0x8000D8` before walking the model
(`0x2113CC–0x211451`, `0x024610`, `0x024A60`, `0x19C480`).

The outer loop advances model X, then Y, with Z innermost, each
ascending from zero. For each horizontal column first scan Z for the
lowest and highest nonblack cell. Initial low is size Z and initial
high is zero. The second scan transforms each cell center at integer
model coordinates plus binary32 0.5 on every axis. Convert the
transformed offset to fixed units through `0x02C460`, add the anchor
multiplied by 65,536, and truncate each signed sum divided by 65,536
to a world block. The translation precedes this final conversion
(`0x211458–0x211604`, `0x2117AB–0x2117F7`, `0x0248A0`).

- A black voxel strictly between the column's two occupied Z extrema
  calls the general writer with black RGB and type byte `0x40`. It
  bypasses the explicit existing-cell protection test and emits no
  day record. Other black voxels make no write
  (`0x211614–0x211687`).
- A colored voxel queries its transformed world cell. A returned type
  byte with bit `0x40` set skips both the write and append. The query
  chooses `(200, 200, 200)` with type `0x01` for missing columns or
  cells below their stored bottom. Above the stored top it chooses
  air for positive Z and implicit sea otherwise. A stored type-zero
  cell at nonpositive Z with bit `0x40` clear also resolves to implicit
  sea. That sea type byte is `0x82`
  (`0x21168C–0x2116EA`, `0x02F7E0–0x02F854`,
  `0x2F9750–0x2F9770`, `0x2F9780–0x2F97A7`).
- An admitted colored voxel calls the general writer with its RGB and
  type `0x46`. It then appends a 20-byte record in visitation order,
  even when the column writer returned for a missing column. The
  record is three signed 32-bit block coordinates, three color bytes,
  type `0x46` and the 32-bit day from world `+0x800160`
  (`0x2116F0–0x2117A6`, `0x086240–0x08627C`).

The general write at `0x04E7A0` needs a loaded column. The underlying
stored-cell write at `0x04E700` preserves an existing bit `0x80`.
An incoming type with its low five bits zero against that bit leaves the
stored cell unchanged. Other incoming types replace its bytes and carry
bit `0x80` forward. A colored placement can store `0xC6` while emitting
`0x46`. Writing below the column's current bottom uses a separate
expansion path and fills upward toward the former bottom with type 6
using the positional color routine at `0x1F9620`. The traced loop
uses per-cell checks without a preceding actor intersection or
overall footprint test.

## 4. Cleanup, loaded zones and local results

The light branch at `0x211803–0x21180A` requires world local-creature
pointer `+0xB8` to be null. Normal offline player registration
populates that pointer (`0x06222C`, `0x087E96`). Both continuations
reach the sprite cleanup call at `0x211929`. It takes the uncentered
rectangle from anchor X and Y through anchor X plus model size X and
anchor Y plus model size Y, inclusive, using the unrotated sizes.

The sprite helper clamps each endpoint up to zero, divides by 256
toward zero, then visits the resulting zone keys in ascending X and
ascending Y. Missing and out-of-range zones are skipped. It visits
the full lists at zone `+0x04` for the loaded zones it enters. A
node with mask `0x02` set at node byte `+0x40` qualifies for a support read at
its fixed position one block down. The native sampler at `0x02F860`
subtracts a block before division from a fixed component with negative
signed high word, then uses signed truncation. The support query uses
the world cell rules from section 3. A low-five-bit type of zero
marks that node for removal. The helper then unlinks the marked nodes
from the resident sprite list. The rectangle sets the zone iteration
bounds. The sprite loop has no separate per-node X or Y rectangle
comparison (`0x1A5CB0–0x1A5F2F`).

After the sprite pass, compute anchor zone X and Y by signed division
by 256 with truncation toward zero. The zone lookup `0x034D10` accepts
each key from 0 through 65,535 and requires the region and zone to be
present. When loaded, the anchor zone receives the 16-byte entry at
zone `+0x3C` with four signed 32-bit values in order, live model
number, anchor X, anchor Y and anchor Z. The zone is marked for a
snapshot. This branch uses the loaded anchor independently of which
individual voxel writes succeeded. The original lock is released
after the zone step, then the queue walk continues
(`0x21192E–0x2119C7`). The entry carries no rotation field.

At world-piece wrap-up the changed-zone set contributes each changed
zone's full placed-object list to result `+0x38`, keyed by the zone
coordinates. After that pass, the result `+0x18` edit list is visited
in order. Each edit's own X and Y divided by 256 toward zero resolve
the zone to receive the 20-byte edit in its history at zone `+0x68`.
Missing zones get no history entry. The edit-history zone can differ
from the anchor zone (`0x22129A–0x221349`, `0x22134B–0x221410`).

The offline arm writes live terrain directly. The frame's incoming
terrain applier at `0x08F242` consumes game `+0x800648`, fed by
packet-4 splicing at `0x06CEE8–0x06CF07`. The incoming placed-object
replacement at `0x08F58D` consumes game `+0x800668`. The checked
routes keep these pending lists separate from the offline world-piece
result at `0x08CF20–0x08CF49`. The kind-7 arm appends no immediate
sound, particle or creature-hit record. Broader presentation and
persistence beyond these traced result boundaries remain dependencies.

## 5. In-world preview

The preview uses the same signed model lookup and is gated by game
`+0x800704` at `0x0B3338`. It rounds live position plus aim to a block
anchor and prepares the draw translation. It sign-extends the same
rotation byte, multiplies by 90 degrees and translates the rotated
model by the negative integer half sizes in X and Y. Both horizontal
dimensions are even for the resolved stock models. This gives the same
horizontal half-size translation as the placement arm for those models
(`0x0B3345–0x0B3549`). Preview drawing ends with the render-state calls
at `0x0B354E–0x0B35CF`. Its checked route resolves and draws the model
independently of the per-cell protection and loaded-zone checks in
the offline write.
