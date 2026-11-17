# Alpha action-79 light inputs

**FINAL / STABLE / HELD**

Scope covers the connected client's action-79 light sample and the original
world records it visits. The outer action gate and subsequent meter use are in
[alpha-connected-actor-turn.md](alpha-connected-actor-turn.md) §3.

Evidence is the original 32-bit Alpha `Cube.exe`, 3,885,568 bytes,
SHA-256 `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Addresses below are RVAs for preferred image base `0x00400000`. Field offsets
are relative to the named native record. Positions use three signed 64-bit
components, with 65,536 fixed units per block. Binary32 denotes single precision.

## 1. The zones shared by one world step

The client world step at `0x20C510` creates an empty ordered set of 32-bit
zone pointers at `0x20C88C–0x20C8A5`. The seed walk at `0x20C8E1–0x20CCFF`
visits the live creature map at world `+0x04`. A seed needs a nonnull
creature and a zero controller byte at creature `+0x60`. Health, distance to
the local creature and local identity are outside this admission test
(`0x20C8F4–0x20C8FF`, `0x20CA3D–0x20CA43`). A creature executing action 79
uses this same set even if its own controller byte is nonzero.

For each seed, the step converts its current X and Y from creature `+0x10`
and `+0x18` to blocks by signed division by 65,536 toward zero. It uses the
low 32 bits of each quotient as a signed block coordinate and divides it by
256 toward zero to obtain the zone coordinate. It probes the inclusive
three-by-three neighborhood from center minus one to center plus one, ascending
X on the outside and ascending Y on the inside. Only nonnull pointers
returned by the resident-zone lookup are inserted
(`0x02F570`, `0x20CA49–0x20CB15`).

A zone covers 256 by 256 blocks. The resident lookup `0x034D10` accepts
signed zone X and Y in 0 through 65,535. It looks up the enclosing region at
X and Y divided by 64. The enclosing-region table at world `+0xBC` is
indexed by 1,024 times region X plus region Y (`0x034A90`). The region's
zone table at `+0x10018` is indexed by 64 times zone X modulo 64 plus zone Y
modulo 64. Empty table entries yield no zone. The zone records themselves
carry signed 32-bit X and Y at `+0x60` and `+0x64`
(`0x1E49C7–0x1E49D2`).

Insertion and traversal use the unsigned zone pointer as key. The insertion
path merges an equal pointer even when separate creatures or probe coordinates
lead to it. Traversal is in ascending pointer order. Thus coordinate order,
probe order and the number of seeding creatures affect membership only as
described above, while the live addresses determine accumulation order
(`0x19F070`, `0x18E824–0x18E846`, `0x18E893–0x18E8BD`, `0x219693–0x2196D9`,
`0x2199C7–0x2199FB`).

The insertion pass ends before the zone-content updates and the per-creature
action turns. All action-79 subjects in one invocation use this same
membership. Contents are read live when the action executes. The set expires
at world-step cleanup (`0x221432–0x221438`). A later invocation, including
another subdivision of the same connected frame, builds its own set.

## 2. Day and terrain input

The action-byte value 79 at creature `+0x68` enters the light route at
`0x2195BA–0x2195BE`. Its day input, read at `0x219611`, is the signed
32-bit world clock at `+0x80015C` (`0x071910`). The same world step has
already advanced it by elapsed milliseconds times 10, or times 100 when
its early walk finds a nonnull controller-kind-0 creature with action 84
and positive health. The early update overrides the result with
32,400,000 ms when the world dword at `+0xA4` is zero. It subtracts
86,400,000 ms for each rollover while the value strictly exceeds
86,400,000 ms (`0x20C580–0x20C6E2`).

The daylight factor starts by converting that signed clock to binary32,
multiplying by 2, dividing by 86,400,000, and subtracting 1. It squares
the result, subtracts the square from 1, and squares again. Each of these
two squares uses `0x011D10`, which promotes the binary32 base and exponent
2 to binary64 for the power call, then returns a binary32 result. The
action multiplies the daylight factor by the terrain return in binary32
to initialize the accumulator (`0x219611–0x2196A0`).

The terrain sample uses the subject's live fixed position at creature
`+0x10`. The helper divides Z, Y and X independently by 65,536 toward
zero and passes the low signed 32-bit block coordinate from each. The
fourth argument of the cell query is a null optional zone override. The
column helper resolves the resident zone from the same X and Y block
coordinates divided by 256. Z selects the actual cell in that column
(`0x19FEA0–0x19FEEE`, `0x02F7E0–0x02F852`, `0x0347A0–0x03486D`).

The horizontal block range is 0 through 16,777,215 on each axis. The
column base at zone `+0xA8` has 32-byte columns, indexed by 256 times
local block Y plus local block X. Column `+0x10` holds its signed
32-bit starting Z, `+0x1C` the 32-bit cell count, and `+0x18` the
cell-data pointer. An in-range cell occupies four bytes at the offset
given by four times the difference between Z and the starting Z
(`0x034831–0x03485B`, `0x02F7D4–0x02F7D7`, `0x02F800–0x02F837`).

| Condition | Cell used |
| --- | --- |
| Out-of-range X or Y, absent zone, or Z below the column | Fallback `0x36AFC4`, initialized to `C8 C8 C8 01` |
| Z at or above the exclusive end of the column, with positive Z | Air fallback `0x36AFC0`, initialized to `FF FF FF 00` |
| Z at or above the exclusive end of the column, with Z zero or below | Water fallback `0x36AFBC`, initialized to `FF FF FF 82` |
| Stored type zero with Z zero or below and byte `+0x03` mask `0x40` clear | The same water fallback |
| All other in-range cells | The stored cell |

The fallback values come from `0x2F9720`, `0x2F9750` and `0x2F9780`.
The action sample interprets the low five bits of cell byte `+0x03` as
type. Type 13 gives numerator 255. Types 0 and 2 give the greater of
5 and unsigned cell byte `+0x00`. Every other type gives zero. The
sample is the numerator divided by 255 in binary32
(`0x19FEF3–0x19FF3D`). With the initialized fallbacks, a missing or
below-column sample is zero and the air and water fallbacks each give
one. This is a single-cell query at the subject's sampled block height.

## 3. The three resident collections

Within each visited zone, the scan completes the `+0x04` collection,
then `+0x0C`, then `+0x30`. List entries are visited forward from the
head. Array entries are visited from the beginning to the end in physical
record order (`0x2196D3–0x2199FB`).

| Zone offset | Live collection | Candidate gate | Position |
| --- | --- | --- | --- |
| `+0x04` | Linked list of static model placements | Mask `0x01` in the low byte of the placement's 32-bit flags at `+0x38` | Placement `+0x08` |
| `+0x0C` | Contiguous array of 0x188-byte world objects | 32-bit type 50 at object `+0x00` | Object `+0x08` |
| `+0x30` | Contiguous array of 0x148-byte world items | Unsigned category byte 18 at record `+0x00` | World-item record `+0x118` |

The gates are at `0x21971F`, `0x21981B` and `0x21990F`. The two array
strides are at `0x0ABC4A` and `0x04B58A`. The zone constructor starts
all three collections empty (`0x224126–0x224199`). All three positions
supply the same three signed fixed-point components.

### 3.1 Static model placements

A zone generator at `0x1CC510` makes placements through `0x1F8960`
and appends them to the list tail through `0x0862E0`. Four verified
factory and append pairs are `0x1D2485/0x1D2497`,
`0x1D27EA/0x1D27FC`, `0x1D2B4F/0x1D2B61` and
`0x1D2EBE/0x1D2ED0`. The placement constructor defaults the 32-bit
flags to 2 and copies the 24-byte position at `+0x08`. It produces
qualifying examples with placement type 54 and flags 1 at
`0x1F8A26–0x1F8A2C`, and type 52 and flags 1 at
`0x1F8A7C–0x1F8A82`. The type-50 placements at `0x1F8A4D` and
`0x1F8AAA` retain default flags 2 and so contribute no light through
this list. The light gate depends on the current flag alone. The
list-node copy retains that flag (`0x1C0401–0x1C040B`, `0x059AB0`).

The static placement draw uses the model table at game `+0x800718`.
Placement type 52 is slot 0x9AA, `torch-green.cub`, and type 54 is
slot 0x9AC, `torch-yellow.cub`. Static placement type 50 is slot 0x9A8,
`wall-skull.cub`. The lookup is at `0x0BD18C–0x0BD19B`, the assignments
are at `0x062AF6–0x062B8D`, and the loaders are at
`0x118327–0x118489`, with the strings at `0x31101C`, `0x31103C` and
`0x31105C`.

These placements qualify for as long as the record is in a visited
zone's live list, the flag is set, and the distance test passes. The
light traversal imposes no timer, owner or model-type check.

### 3.2 Streetlight world objects

The stock type-50 world object resolves through the game model table at
`+0x800724` to model slot 0x9E1, `street-light02.cub`. Type 51 resolves to
slot 0x9E2, `street-light01.cub`. The action-79 object pass admits type 50.
The model assignments are at `0x063187–0x0631EF` and the loader calls
are at `0x119357–0x1193D4`, with strings at `0x311380` and `0x311394`.
The object render walk reads this same type table at
`0x0B2CA8–0x0B2D11`. Its separate local illumination pass also checks
zone object type 50 at `0x0B08C3`.

Zone construction at `0x1E4850` has a streetlight branch for selector
value 11 (`0x1EBD43`). After its preceding site checks, it requires a draw
with remainder zero modulo 16 and checks seven cells in the chosen
column from its candidate Z through Z plus six. Each cell must have type
0 or 2 (`0x1EBD80–0x1EBDFC`, `0x03B480`). It then reads the
generation column's binary32 `+0x08` value through `0x1C8350`,
with the zone under construction passed as the column override.
A value greater than binary32 0.8 assigns type 51. At or below that
threshold it assigns type 50 (`0x1EBE1A–0x1EBE7B`).
The chosen block coordinates become the record's fixed position.
The generator appends the record to the zone array through `0x1A0E00`
(`0x1EBE92–0x1EBEFB`). The constructor at `0x18EAA0` initializes its
32-bit `+0x34` elapsed field and 64-bit `+0x40` reference to zero.
A new zone becomes resident when this generation routine publishes its
pointer at `0x1EE05A`.

At the earlier per-zone object step, elapsed milliseconds are added
to `+0x34` before the actor turn (`0x20D3B0–0x20D3C3`). The connected
path for the generated zero-reference object proceeds to the next array
entry after the world `+0xB4` byte test at `0x20D8F5`. The light scan has
no age test. Its current type, fixed position and continued array
membership determine whether the object qualifies.

A pending packet section-6 record can change an existing world object
in place in the connected frame. It resolves the loaded zone, checks
a nonnegative signed index strictly below the array count, and
addresses that indexed `+0x0C` entry. The copy includes the full
32-bit type and fixed position, and the entry remains in the same
collection slot (`0x08F435–0x08F536`, `0x04B560`, `0x068670`).
This can change whether the entry contributes on a later world step.

### 3.3 Candle world items

Category 18 is the Candle item class. The ordinary model selection uses
unsigned item seed `+0x04` modulo three for `candle01.cub`,
`candle02.cub` or `candle03.cub`, in slots 0x845 through 0x847.
Subtype byte `+0x01` equal to 1 instead selects `undead-candle1.cub`,
`undead-candle2.cub` or `undead-candle3.cub` in slots 0x848 through
0x84A. The action-79 gate uses category 18 for all those records
(`0x0EC40A–0x0EC417`, `0x0EDD5F–0x0EDD97`, `0x114795–0x114A68`,
model strings at `0x31004C–0x3100D8`).

The earlier world-item step bypasses its type-16 branch for a Candle
but still decrements the positive signed 32-bit timers at item record
`+0x140` and `+0x13C` by elapsed milliseconds, flooring each timer at
zero. In the connected world it advances to the next entry when the
second timer reaches zero. A Candle with expired timers can still
qualify for light (`0x20CF53`, `0x20D1C9–0x20D222`, `0x20D29E–0x20D2D1`).

Connected packet section 7 replaces the entire `+0x30` array of its
loaded zone with the received world-item records in their received
order. An empty snapshot clears the array. The update is consumed
even when the zone lookup misses (`0x08D5F0–0x08D714`, `0x04BE20`,
`0x057DB0`). Section 7 precedes the section-6 object update. Both
apply after the last world-step call on a connected frame that runs
the world step (`0x08CF20–0x08CF49`). Their changes are visible to
later world steps.

## 4. Addition order and live creature lights

For each candidate, the distance helper subtracts the subject's signed
64-bit fixed position from the candidate's fixed position. It converts
each component to binary32, then multiplies each by binary32 1/65,536.
The rounded X and Y squares are added, then the rounded Z square is
added, in binary32 (`0x02C7A0`, `0x02C4A0`, `0x024860`).
All three zone collections require squared distance below 256.
Each accepted record multiplies the squared distance by 1/256,
subtracts that product from 1, squares the result, multiplies by 0.5,
and adds the result to the accumulator. These arithmetic operations
are separately binary32. Each entry in range contributes in traversal
order, including records at the same coordinates. When every zone has
been scanned, the action caps the accumulator at 1
(`0x21974C–0x219794`, `0x219847–0x21988F`, `0x21993E–0x219986`,
`0x219A0D–0x219A2A`).

The next walk uses the live creature map at world `+0x04`, in
ascending signed 64-bit creature ID order. It compares the high 32
bits as signed and the low 32 bits as unsigned for equal highs. The
map node payload stores the eight-byte ID followed by the native
32-bit creature pointer. The walk takes the successor in the map
(`0x02F000`, `0x077DB0–0x077DC5`, `0x1C3EA0`, `0x219A2A–0x219B1C`).

A nonnull creature qualifies for the distance test when its 16-bit
status field at `+0x124` has mask `0x0200` set. The source position is
its current fixed position at `+0x10`. This admits the subject itself
and any map entry with the flag regardless of health, controller
kind, ownership or membership in the visited zones. There is no
equipment read in the light walk (`0x219A6D–0x219AB3`).

The creature distance is calculated as for the zone entries and its
squared value must be below 100. The contribution is the square of
one minus squared distance divided by 100. The division, subtraction,
square and addition are binary32. Self at zero distance contributes 1
when flagged. Creature lights are added after the zone-light cap and
the sample reaches its action-79 consumer without a second upper cap
(`0x219ABB–0x219B71`). The traversal and distance tests draw no
random numbers.

The creature pointers, flag and positions are read live. A source can
already have run its turn or still await it when the subject samples.
For example, the later equipment check of an actor's own turn clears
mask `0x0200` when the equipment category byte at `+0xDF0` differs
from 24 (`0x21F026–0x21F03B`). That equipment category resolves to
model slot 0x981, `lantern02.cub` (`0x0EDDDF–0x0EDDFE`,
`0x117797–0x1177CA`, string at `0x310D7C`). The action-79 sample reads
the bit as it exists then, so a later clear affects subsequent samples.
Removal from the live map also ends that creature's eligibility.

## 5. Evidence bounds

The complete writer sets for static placement flags and creature
light bits, further world-item admission and removal routes, and full
terrain generation remain open. The streetlight placement evidence
covers the branch at selector 11 and its clearance and type selection
after earlier site checks.
The contract describes the action-79 brightness sample. Native
visible illumination also has its own traversal and draw rules.
