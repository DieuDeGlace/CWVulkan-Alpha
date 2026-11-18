# Alpha stock blueprint state writers and persistence

**FINAL / STABLE / HELD**

The supported original image is the 32-bit Alpha `binary/Cube.exe`, 3,885,568
bytes, SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`,
with preferred image base `0x00400000`. Addresses are client RVAs. Controller,
world, actor and zone offsets refer to their respective object bases. The
controller embeds its world at `+0x2E4`. This contract covers activation and
selection evidence, live placement-list delivery and the checked zone save
round trip. [Conditional stock placement](alpha-stock-blueprint-placement.md)

## Controller mode and startup inputs

The byte at controller `+0x800704` is initialized to zero at `0x05A09B`.
There are six occurrences of the exact 32-bit displacement in original
executable text. That store is one. All five others test the byte.

| Test RVA | Path using the byte |
| --- | --- |
| `0x07CCFF` | Secondary mouse-button route to the local rotation increment |
| `0x09797D` | Rising-edge R route to a placement request |
| `0x09B18D` | Frame aim movement |
| `0x09B5B1` | Branch past ordinary slot processing |
| `0x0B3338` | In-world model preview |

The same absolute field location expressed from the embedded world would
use displacement `+0x800420`. Its four-byte literal has no executable-text
match. The adjacent outgoing list has only two header words from controller
`+0x8006FC` through `+0x800703`. Construction, append, send cleanup and
destruction change those words or separately allocated nodes. The nullable
UI pointer at `+0x800710` is stored at `0x05A0A2`. Its other 190 exact
displacement uses in executable text load or push the pointer value.
The adjacent receive headers end at `+0x800697`. These are separate storage
ranges. Evidence includes `0x05A06D–0x05A0A8`, `0x067802–0x067842`,
`0x069D19–0x069D2B`, `0x069EB7–0x069EF5`, `0x09C210–0x09C216` and
`0x2E5156–0x2E515F`.

The startup `options.cfg` parser at `0x0CE6E0` is called on the settings
block at image `0x36B1D8` from `0x0C8DA5–0x0C8DAF`. Its twelve named
scalar destinations cover fullscreen, resolution X and Y, anti-aliasing,
render distance, sound and music volume, camera speed and smoothness,
language, invert Y and minimum timestep. All extracts address that settings
block between offsets `+0` and `+0x2C`. The settings writer at `0x0CEF80`
emits those same keys. The mode byte is initialized separately in the
controller constructor after parsing.

The command-line route at `0x0C8F06–0x0C8FA3` considers the first argument
after the executable. Its native comparison with the wide `server` literal
at `0x302FB4` sets a startup flag in the seventh argument slot of the
constructor call at `0x0C8FF9`. The alternate text path supplies the eighth
slot, copied to the controller string at `+0x800598` by `0x059F76–0x059FB1`.
Both inputs reach the zero initialization at `0x05A09B`. The original
constructor instruction range contains no direct read of the seventh stack
argument at frame `+0x20`.

World construction registers the stock model assets in the separate
world table at `+0x800154`. The block at `0x1933EB–0x193436` creates
key zero and loads `framework-floor-wood.cub` for that entry. Thus the
initial actor value zero is an actual stock model key when the content
resolves. The registration writes to the model table. The startup
`build-cursor.cub` load uses controller `+0x800734`, another separate
destination (`0x0634C6–0x0634FB`).

## Local selected model lifetime

The signed model byte is actor `+0x18D` and the adjacent rotation byte is
actor `+0x18C`. Within the creature-state block starting at actor `+0x10`,
their offsets are `+0x17D` and `+0x17C`. New state and full reset clear both
with a two-byte write at `0x03C276` and `0x0466A0`. The client constructor
allocates and constructs the local actor through `0x03B690` before storing
its pointer at controller `+0x8006D0` (`0x062019–0x062022`). Registering
that pointer at world `+0xB8` uses `0x06222C` and `0x087E96`.

State assignment and copy construction transfer both bytes from their
source at `0x04B22B–0x04B23F` and `0x06623A–0x06624E`. Character-file
restore first assigns freshly initialized state at `0x04BF84–0x04BF93`.
The following actor scalar reads and equipment block at actor `+0x300`
leave the model/rotation pair at its initialized value
(`0x04BF98–0x04C16E`). The menu copy at `0x066E2D` takes current local
state as its source and the indexed character record as destination.
The copies at `0x0823B1` and `0x08762B` also take the local state into
separate constructed actors.

For a login identity change, `0x070211–0x070221` makes a snapshot of the
previous local state. The client may take an existing resident from the
world map at `0x0702D1–0x0702DE`, or create and insert one at
`0x0702E0–0x07032B`. Both branches assign the previous local snapshot
to the resulting local actor at `0x070333–0x070343`, including the pair.
The outgoing state path at `0x069EF7–0x069F19` snapshots the local state
for serialization and copies it to a local comparison record after the
send at `0x069FF6–0x06A00A`.

The two packet state decoders can write the two model/rotation bytes in
their state record via separate masked fields at `0x0CD253–0x0CD27B`
and `0x0CD847–0x0CD879`. The packet-0 resident application at `0x06BDFF`
is bypassed when the resident equals controller `+0x8006D0`
(`0x06BCC5–0x06BCCB`). Packet 1 has the same local-pointer exemption
at `0x06C69A–0x06C6A0`, before the resident application at `0x06C96C`.
The preceding `0x06C331` copy is in the opposite direction. It copies
resident state into a temporary ID-indexed state container through
`0x068B70`.

The only two direct byte stores bearing displacement `+0x18D` are
`0x188C75` and `0x18A881`. Their receiver is the voxel widget identified
by its constructor, vtable at `0x31A664` and original RTTI locator at
`0x3480DC`. Its `0x188500` entry writes an item record at widget `+0x180`.
A second preview object is constructed at `0x064739` and stored at
controller `+0x800964`. Its vtable at `0x2FD364` and RTTI locator at
`0x346D70` identify the blueprint preview widget. Recipe preview updates
copy an item into that widget at `+0x16C` and update its following preview
data (`0x07BBC7–0x07BBF6`, `0x0A1DDA–0x0A1E05`). These are widget item and
preview destinations. The in-world drawing gate instead reads signed
model from the local actor at `0x0B3351`.

## Conditional placement admission

The traced key writers update controller key state. The physical R sample
is at controller `+0x0F`, and the frame saves the previous sample at
`0x09B593–0x09B59C`. The nonzero-mode arm appends a kind-7 request only
on an R rising edge (`0x097959–0x0979C7`). With that mode enabled,
secondary button index 1 increments rotation alone as an eight-bit write
at `0x07CD0E`. The aim path writes the aim components at actor `+0x160`.
These are consumers of the enabled mode. The startup, configuration,
widget, input, actor lifetime and packet routes above establish no stock
transition from zero into that mode and no fresh local model-choice writer.

Kind 7 is a valid compiled queue case. The dispatch at
`0x210535–0x210546` indexes the table at `0x222C20` with kind minus one.
Its seventh pointer selects `0x2112F7`. This arm requires offline world
byte `+0xB4` equal to zero and a model found by the signed live actor byte
at `0x211312` through `0x198840`. The request contains its kind. Actual
selection, rotation and aim come from live actor state at consumption.
The request producer's mode gate and the world consumer's model and world
gates are distinct. Ordinary stock input reachability of this case remains
unverified.

## Live anchor zone and world result order

Zone construction zeros the placed-model vector header at
`+0x3C/+0x40/+0x44` (`0x22419A–0x2241A8`). After terrain work and sprite
cleanup, a successful loaded anchor-zone lookup uses anchor X and Y each
divided by 256 toward zero. The placement arm appends at the end of this
vector through `0x20AAC0`. The 16-byte entry is four signed 32-bit values
in order, the live model number sign-extended anew, anchor X, anchor Y,
anchor Z. The entry contains no rotation. The arm then inserts that zone
pointer into the world piece's changed-zone tree before releasing the
world mutation lock (`0x21192E–0x2119C7`). A missing anchor zone skips
both the append and the changed-zone insertion.

The changed-zone tree orders zone pointers by unsigned address
(`0x19F070`, `0x18E824–0x18E846`). At `0x22129A–0x221349` the world
piece visits the tree in order and appends a snapshot for each member to
result `+0x38`. The snapshot starts with the zone's signed X and Y from
`+0x60/+0x64` in that order. It then carries a copy of the full 16-byte
entry sequence from zone `+0x3C` up to `+0x40`, in increasing vector order.
The copy is separately owned. Evidence is `0x208F60–0x208FE1`,
`0x208B30`, `0x208DB0` and `0x20AA70`.

After the snapshots, the piece walks result `+0x18` in edit order. Each
edit selects its own zone using signed X and Y divided by 256 toward zero.
It appends the 20-byte edit to that loaded zone's history at `+0x68`
through `0x04D4B0`. The payload is signed block X, Y, Z, then four voxel
bytes in R, G, B, type order, then the 32-bit day. For an admitted colored
stock placement the emitted type is `0x46`. The black infill path creates
no edit record. Edit-history placement and anchor-vector placement can
therefore affect different zones (`0x211614–0x2117A6`,
`0x22134B–0x221410`).

The frame also has a distinct incoming placement-list path at controller
`+0x800668`. It resolves the packet snapshot's zone coordinates, clears
the loaded zone's placed vector and copies the incoming sequence to that
vector (`0x08F58D–0x08F677`). The save writer below takes a live zone
pointer and has its own admission and payload.

## Zone save and restore order

The offline zone writer at `0x1A5050` receives a live zone pointer. It
requires a nonempty world name at world `+0x94`, a present zone and either
a nonzero zone byte `+0x75` or a nonempty edit-history vector at
`+0x68/+0x6C`. The anchor-vector append and changed-zone insertion do not
write `+0x75`. Anchor-list content alone supplies no save admission.
Observed offline save calls include `0x1A4960`, `0x1A5585` and `0x193F55`.

The writer uses the world database at world `+0xAC`, opened by the
offline world setup at `0x1A56D8–0x1A5725` with the `Save/world_` prefix,
world identifier and `.db` suffix. Its zone key is `zone` followed by
the zone X through the signed integer stream, underscore and zone Y.
The key uses resident `+0x60/+0x64`. The writer passes a fresh binary
buffer from `0x04A8A0` and `0x04E340` to the database write at `0x1A51DB`.

The buffer order below is for ordinary valid native storage. Counts are
32-bit and multibyte values are little-endian.

| Order | Zone payload |
| --- | --- |
| 1 | 32-bit format value 1 from the serializer context at `0x04A8A8`. |
| 2 | Count for the zone `+0x30/+0x34` vector, or zero when zone byte `+0x75` is zero. For each admitted `0x148`-byte live element, write its first `0x118` bytes, then `0x18` bytes from element `+0x118`, then the 32-bit words at `+0x130/+0x134`, the byte at `+0x138`, and the 32-bit words at `+0x13C/+0x140/+0x144` in order. |
| 3 | Edit-history count and the contiguous 20-byte entries from zone `+0x68/+0x6C` in vector order. |
| 4 | Count of pointers from zone `+0x18/+0x1C`, then the two 32-bit values at `+0x38` and `+0x3C` of each pointed record, in pointer-vector order. |
| 5 | Count of static records from zone `+0x0C/+0x10`, then the byte at each record's `+0x30`, in `0x188`-byte resident order. |

This is the complete observed zone serializer at `0x04E340–0x04E6FB`.
The `+0x3C` in row 4 is relative to the pointed record. The zone's own
placed-model vector has no field in this payload. The result snapshot
also supplies no argument to this save path. The colored edit history
is the traced persisted contribution of conditional stock placement.

For a newly generated zone, the offline and nonempty-name checks at
`0x1EDD69–0x1EDD83` admit a read of the corresponding zone key. A successful
lookup at `0x1EDE0D` calls the restore decoder at `0x1EDE46` on the newly
constructed zone. The zone is subsequently published at `0x1EE05A`.
The decoder reads the format value and the `+0x30` section first. It then
reads the edit count and iterates the 20-byte entries in saved order.
Each edit is admitted if its signed day is negative or at least the
current signed world day minus three, with a 32-bit subtraction. For
each admitted edit the decoder calls the ordinary voxel writer with the
resident zone, then appends that same record to zone `+0x68`
(`0x04D0E8–0x04D189`). World day itself is the first 32-bit value in the
separate `time` row (`0x20C785–0x20C7F4`, `0x1A578E–0x1A57DB`).

The later restore sections apply the two per-record words and the static
property bytes when their saved counts match the generated counts, in
resident order (`0x04D2CB–0x04D428`). The full decoder at
`0x04CDB0–0x04D45B` has no reconstruction of the model-and-anchor entries
from the saved grid edits. The zone constructor provides the empty
placed vector, with the save restore leaving its contents untouched.

## Evidence boundary

The writer audit used the supported executable text, matched original
instruction exports, direct disassembly for omitted ranges, and original
pointer tables and literals. The bounds above cover exact displacement
encodings, the neighboring header and widget routes, and the named
configuration, lifetime, packet and save paths. Arbitrary computed-address
stores, indirect callers outside these paths, external state changes,
the server image and runtime acceptance remain open. The zone round trip
describes ordinary stock save buffers and stable native zone structures.
