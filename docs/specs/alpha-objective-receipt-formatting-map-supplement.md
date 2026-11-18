# Alpha objective receipt text and region requests

**FINAL / STABLE / HELD**

The source image is `binary/Cube.exe`, 3,885,568 bytes, x86 PE machine `0x014C`,
timestamp `0x51EA955E`, preferred base `0x00400000`, image size `0x003BC000`,
SHA-256 `84A7A132A84D4282338E7EA45A1940D32066D64E39CBAFED8F2418D5A6DC30BF`.
Executable addresses below are RVAs. The findings are from static image evidence.

## Live descriptor seen by state two

The earlier receipt pass at `0x08FDC3` walks the client list at `+0x800690`.
Each record begins with signed cell X and Y dwords at `+0x00` and `+0x04`.
Lookup `0x087DA0` admits cell axes `0..8191`. The parent is the terrain region
at the cell pair divided by eight, and the cell is its 104-byte record beginning
at region `+0x14018`, with Y contiguous. For a present cell, `0x08FE57` applies
the incoming descriptor at receipt `+0x10` to live cell `+0x2C`. Both the
application and the independent request-latch clear below occur before the
later state checks. Receipt type and state do not gate this descriptor copy.

The copy transfers five initial dwords, two individual bytes at descriptor
`+0x14` and `+0x15`, and four dwords beginning at descriptor `+0x18`. The two
padding bytes at descriptor `+0x16` and `+0x17` are skipped. The same receipt
list is subsequently appended in forward order to the local late-walk list
at `0x0905B8`. The source client list is cleared at `0x09064C` before the
late walk begins at `0x090651`.

Each late state-two record independently requires receipt dword `+0x18`
nonzero, receipt byte `+0x25` equal to two, and a successful fresh cell lookup
using receipt `+0x00` and `+0x04`. The formatter at `0x090777` receives the
live cell pointer. It reads live cell `+0x30` for the name seed, `+0x34` for the
objective template type, and `+0x38` for the creature type. These values come
from receipt `+0x14`, `+0x18` and `+0x1C` of the descriptor most recently
applied to that cell. Therefore, when several receipts target the same cell
in one pass, every qualifying state-two visit formats the live descriptor
left by the last successful copy in encounter order. A later state-one or
zero-type receipt can supply that descriptor. The gate for displaying each
result still reads that visit's own receipt type and state. A live type with
no template can produce empty text while the state-two feedback calls run.

## Objective text call inputs

At `0x09073A` the client calls `0x236810` on the UI tree referenced by client
`+0x800868`, with wide key `shine` and literal zero. At `0x090777` the input to
`0x077FA0` is the client, a temporary output wide string and the live cell.
That formatter prepares the following localization inputs.

- It makes `@name` with the world name helper `0x1A0ED0`, passing cell dwords
  `+0x30` and `+0x38` in that order. The helper uses the former as its name
  seed and the latter to select the creature-dependent name family.
- It supplies `creature` through `0x194C80` with cell dword `+0x38`, two zero
  qualifiers and the same localization variables. For `scenery`, it passes
  the live cell to `0x1953A0`, which looks up the two dwords at `+0x18` and
  `+0x1C`. If the scenery template requests a generated name, that helper
  uses cell dword `+0x20` with name-family selector `0xFFFFFFFF`. These three
  scenery fields precede the receipt descriptor at cell `+0x2C`.
- It offers the selected wide template key, the prepared variables and an
  initially empty destination entry list to localization routine `0x0E4A20`
  on client `+0x314`. The combination of live type and key comes from the
  original table at `0x0787B8`.

| Live type | Localization key |
| --- | --- |
| 1 | `objective:monster` |
| 2 | `objective:rareboss` |
| 3 | `objective:villagemonster` |
| 4 | `objective:villagerareboss` |
| 5 | `objective:dungeon` |
| 7 | `objective:invasion` |
| 8 | `objective:sceneryinvasion` |
| 9 | `objective:villageinvasion` |
| 10 | `objective:caveinvasion` |
| 11 | `objective:riverinvasion` |
| 12 | `objective:gang` |
| 13 | `objective:gangboss` |

Type 6 and every value outside the listed types have no template here. The
formatter joins the resulting tokens from the first localized entry. It
inserts a single space between words. A leading token, a token immediately
after a hyphen, and a token consisting of `.`, `-`, `,`, `;`, `!`, `?` or `:`
receive no preceding space. The returned wide string is supplied at
`0x090795` to `0x236A00` on the same UI tree with wide key `objective` and
literal one. The next feedback call at `0x0907D7` dispatches sound ID `0x1E`
(`missioncomplete.wav`) with position pointer client `+0x140` and two
binary32 arguments each equal to `1.0` (`0x3F800000`).

## State-one localization and subsequent map writes

In the actor branch with flag `+0x7E` bit `0x80` clear, a usable dialogue pool
entry admits preparation. Actor subtype `0x84` takes its own `innkeeper`
formatting path. For another subtype, the client builds the signed working
zone pair from actor dwords `+0x1DC` and `+0x1E0`. If the corresponding
resident cell's level at `+0x24` falls outside the controlled actor's level
plus or minus two, both working coordinates are set to `-1`. The resident
cell lookup uses the working pair divided by eight with signed truncation
toward zero. A lookup miss keeps the pair. The level compare and replacement
at `0x08861F..0x088643` precede the localization call.

At `0x088799`, `0x0E4BD0` receives the world at client `+0x2E4`, actor mode
byte `+0x1D8`, the working pair and third dword, and the destination dialogue
list, using localization object client `+0x314`. The third dword reaching
this call is the current actor dword `+0x08`. The preliminary read of actor
`+0x1E4` is overwritten before the call. At `0x0E4C22` the preparer reseeds
`MSVCR110.dll` `srand` with that third dword, then looks up the live terrain
cell for the working pair divided by eight. Its direct random choices are
as follows. Import RVAs for `srand` and `rand` are `0x2FC4B4` and `0x2FC468`.
The substituted `(-1, -1)` pair truncates to cell `(0, 0)` for this terrain
lookup and remains out of bounds for the subsequent map entry lookup.

- Mode 4 with a resident cell whose `+0x18` dword is nonzero prepares the
  `scenery` variables from that cell through `0x1953A0` and localizes
  `explored`. The preparer has no direct draw after the reseed on this
  branch. A missing cell or zero `+0x18` goes to the general branch.
- Mode 3 uses one `rand` result modulo the count of creature dwords between
  world `+0x88` and `+0x8C`. It supplies the selected creature to `creature`
  with two zero qualifiers and derives `food` from item kind `0x14` with
  the low byte of that creature as the item subtype. It localizes `petfood`.
- The general branch makes four direct draws in order. A draw modulo five
  selects the `itemvendoritem` kind/subtype pair from `(0x0B, 0x0C)`,
  `(0x18, 0x00)`, `(0x17, 0x00)`, `(0x17, 0x01)` and `(0x01, 0x07)` in
  that order. Choice one also sets item byte `+0x0D` to one. A draw modulo
  the creature list count supplies `pet` from the same world list as mode
  3. A draw modulo eleven supplies `monster` from creature IDs `0x6C`,
  `0x6D`, `0x72`, `0x73`, `0x77`, `0x6F`, `0x71`, `0x70`, `0x74`, `0x6E`
  and `0x75` in that order. The last draw modulo 30, plus one, supplies
  the decimal suffix of the wide localization key `random:`.

Only after the non-innkeeper preparer returns does `0x0887AA` call `0x1FC160`
to mark the working zone pair on the map at client `+0x800D44`. At `0x0887BB`
it fetches the existing map entry through `0x202440` for that same pair. A
destination entry and a live terrain region are both required for the map
descriptor transfer. The source is the 16-byte zone record in the terrain
region array at `+0x18`, selected by the working pair's remainders modulo 64,
with Y contiguous. The destination is map entry `+0x10` through `+0x1F`.
The map table at map object `+0xB0` provides existing entries for coordinates
`0..65535`. Neither the map mark nor the descriptor transfer supplies the
preparer's scenery fields. Both precede the dialogue binding gate at
`0x08887B`, so a later binding failure can leave the map work in place.

## Terrain request latch and candidate bounds

At `0x08FE5C..0x08FE89`, each incoming cell pair is divided by eight with
signed truncation toward zero. This independently checks the parent even
when the descriptor copy failed to find a cell. `0x034A90` indexes the
resident terrain region pointer table at world `+0xBC`, with X stride 1024
and Y contiguous, and bounds each region axis to `0..1023`. The embedded
world begins at client `+0x2E4`, so this is the same table as client `+0x3A0`.
When the entry is present, the client clears its region byte `+0x08`. The
map backing table at client `+0x800D44` plus `+0xB0` is a separate object.

The candidate list for `0x09D121..0x09D215` is built at
`0x09CFB3..0x09D10B` in client `+0x2C4`. When client dword `+0x8006C8` is
zero it contains the controlled actor's zone. Otherwise the client walks
the world actor list at client `+0x2E8` from begin to end and adds the zone
of each nonnull actor with byte `+0x60` equal to zero. Actor X and Y are the
signed 64-bit values beginning at actor `+0x10` and `+0x18`. The native
conversion divides by 65,536, then divides the signed low 32 bits of the
quotient by 256. Both divisions truncate toward zero. There is no camera
or map-window rectangle in this candidate walk.

The loop first checks candidate zone residency through `0x034D10`. This
requires zone axes `0..65535` and a region in the same resident table. It
reads the corresponding zone pointer in that region at `+0x10018` with Y
contiguous and uses a separate append to client `+0x2CC` when the zone
exists with byte `+0x74` nonzero. The parent-region request at `0x09D215`
has an independent gate. For each candidate zone, the client divides both
axes by 64 with signed truncation toward zero, looks up the parent through
`0x034A90`, and appends the region pair to client `+0x2D4` if the resident
parent's `+0x08` byte is nonzero. It rebuilds this request list each pass.
The region lookup bounds are `0..1023` after division. The parent request
has no separate candidate-zone range check and does not depend on the zone
append. The list append at `0x086560` permits repeated region pairs when
multiple candidates share a region. This evidence does not identify other
request producers or establish when these queued entries are sent.

## Evidence limits

`binary/Cube.exe` and `decompiled/client/assembly.txt` identify the frame
routine `0x088EE0`, word and byte fields, calls, literals, and branch tables.
Supporting routine views are `decompiled/client/functions/00468620.c`,
`00487da0.c`, `00477fa0.c`, `005a0ed0.c`, `005953a0.c`, `004e4bd0.c`,
`005fc160.c`, `00602440.c`, `00434a90.c`, `00434d10.c`, and `00486560.c` in
the same directory. Numeric choices and caller data flow were checked in
the original image. Exact localized wording, downstream localization
draw counts and on-screen timing remain outside this static contract.
