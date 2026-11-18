# Alpha client elapsed adjustment and world readiness

**FINAL / STABLE / HELD**

Independent source ruling, 2026-11-17. This closes the explicit writer and
observed input-route audit for controller byte `+0x8006E5`. It also
records the requested and active world identities that control admission
to the client's world calls.
[World-call phase boundary](alpha-world-call-phase-boundary.md) and
[connected client receipt order](alpha-rulings-connected-client-receipt-order.md).

| Reference | Supported original image | SHA-256 |
|---|---|---|
| C | `binary/Cube.exe` | `84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf` |
| S | `binary/Server.exe` | `1ffbcbce251e1506a63c9ce18ab0009d5553fcbb9e6202be2bc3a7d870d34fb1` |

Both images have preferred base `0x00400000`. Image addresses below
are RVAs unless stated otherwise. The evidence is the original
executable bytes, PE imports, native table contents and scoped
instruction decoding. Matched original instruction exports were used
where available. Field offsets are relative to the object named.

## Controller lifetime and explicit writers

The normal client driver allocates one controller of size
`0x01001018`, constructs it with C `0x059C40`, and publishes the
returned pointer in the display driver at C `0x36B1C8`. The
constructor's 16-bit store at C `0x059FF7` writes zero to both
controller `+0x8006E4` and `+0x8006E5`. Neighboring stores initialize
the separate byte `+0x8006E6` and the dword `+0x8006E8`.

The sole explicit occurrence of the four-byte displacement
`0x8006E5` in the C image's executable text is the read in the
frame at C `0x08C104`. The scan covered the full image text, from
RVA `0x001000` for `0x2FAABF` bytes. It also inspected the
preceding 15 starting offsets through the byte to cover explicit
stores up to 16 bytes wide. The embedded world begins at controller
`+0x2E4`, so the scan also checked the equivalent explicit range
from world `+0x8003F2` through `+0x800401`. The constructor's
zeroing store is the only decoded explicit store in those ranges
whose footprint reaches this byte. Later stores to the preceding
byte have one-byte width.

Evidence at C `0x0C8FB4–0x0C900A`, `0x059FD9–0x05A007`,
`0x07B81E`, `0x07DFED`, `0x07E777`, `0x07E7DB–0x07E7EB`,
`0x08C104`, and `0x097424`. The neighboring interface bodies
omitted from the export were decoded directly.

## Stock input and configuration routes

C registers the window procedure at `0x0C83F0`. Its keydown case
for window message `0x100` uses controller virtual slot `+0x08`,
whose original table entry resolves to C `0x07E1B0`. In its
ordinary key path, virtual key `0x4D` (M) toggles byte
`+0x8006E4`. Virtual key `0x1B` (Escape) clears that byte and
changes a separate interface byte. The Pause virtual key
`0x13` maps to the default continuation on this dispatcher.
The direct interface write at C `0x07B81E` and the frame-side
write at C `0x097424` also set `+0x8006E4` alone.

Evidence at C `0x0C8B29–0x0C8B88`, `0x0C845B–0x0C84D5`,
`0x059CC3`, virtual table cell `0x2FFDD0`,
`0x07E708–0x07E7EB`, and key tables `0x07E92C` and
`0x07E95C`.

When the text entry is active, its explicit slash-command
comparisons are `/name`, `/connect`, `/disconnect`, and
`/namepet`. The later ordinary text path can enqueue text for
the connected sender. The name handlers operate on name state,
and the connection handlers operate on socket and world state.
These traced branches yield no activation route for
`+0x8006E5`. Evidence at C `0x07E289–0x07E2BA`,
`0x07E39D–0x07E6D0`, with command strings at `0x301040`,
`0x30104C`, `0x301060`, and `0x301078`.

The stock `options.cfg` reader runs before controller allocation
with a separate configuration object at C `0x36B1D8`. Its explicit
`minTimeStep` key reads a value into that object at `+0x2C`.
The controller constructor copies that field to controller
`+0x19C` as part of the configuration copy. The outer display loop
compares its signed iteration duration with `+0x19C`. A duration
below that threshold sleeps for the difference. This is the proved
file-backed pacing control. It operates at the outer-loop end.

Evidence at C `0x0C8DA5–0x0C8DAF`, `0x0CE6E0–0x0CEF31`,
strings `0x303194` and `0x3032E8`, `0x0CEEE0–0x0CEF05`,
`0x0C8FD9–0x0C8FF9`, `0x059DD8–0x059E27`, and
`0x0C9432–0x0C945C`.

The traced stock menu, keyboard, text and settings paths supply no
confirmed nonzero writer for `+0x8006E5`. An indirect writer using
a computed address or a bulk operation across this byte remains
unresolved. The explicit scan is bounded by the encoded
displacements and store widths above. Native runtime activation
and any user-facing designation for the byte remain open.

## Guarded elapsed behavior

The display driver samples `timeGetTime` and supplies the low
32-bit difference from its prior sample. A zero prior sample is
replaced with the current sample for that difference. The frame at
C `0x088EE0` stores the argument in its signed elapsed local.
The guarded adjustment at C `0x08C104` reads controller
`+0x8006E5`.

| Byte or computed elapsed | Effect at the guarded site |
|---|---|
| Byte zero | The stored elapsed value continues unchanged at this site |
| Byte nonzero and signed quotient of elapsed by four at least one | Store the quotient, with division truncated toward zero |
| Byte nonzero and that quotient below one | Store one and invoke `Sleep(5)` |

The same elapsed local is later read and copied to controller
`+0x8006E8` before admission. The apparent stack-offset match
at C `0x08C80E` occurs after a separate argument push. It
stores a controller subobject address in a different local.

Evidence at C `0x0C860D–0x0C8633`, `0x088F31–0x088F34`,
`0x08C104–0x08C134`, `0x08C800–0x08C814`,
`0x08CE3E–0x08CE42`, and sleep import `0x2FC084`.

## Requested and active world identities

The requested seed is controller `+0x800A50` and the requested
narrow name object is controller `+0x800A54`. The active seed
and name are world `+0x800164` and world `+0x94`, which are
controller `+0x800448` and controller `+0x378`.
The ordinary controller constructor initializes the requested pair
with seed 123 and an empty name. Its later startup path samples
`timeGetTime`, stores that sample as the requested seed, and
supplies the new seed and the requested name to the active-world
setup. Local world selection paths at C `0x082939` and
`0x084209` take the seed and name from the selected world entry
and call the request routine. An observed disconnect path
supplies the current active pair to the same request routine.

Evidence at C `0x05A4D5–0x05A4FF`, `0x05A72D–0x05A741`,
`0x082925–0x082939`, `0x0841E2–0x084209`, and
`0x071B21–0x071B35`.

### Connected request publication

The dedicated backend takes its seed from active world
`+0x800164` at construction and stores it at backend `+0x08`.
Its join/send path emits a four-byte type 15 and then the
four-byte seed. The C receive dispatch maps type 15 to C
`0x06B8BE`. On the completed read, the client forms the
name from the narrow prefix `online_` followed by the seed
formatted as a signed integer. In this message the name is
constructed locally from the seed.

The receiver compares the new seed, then the new name, with the
requested pair. Matching values bypass the request call.
Either difference calls C `0x06F620` with the received seed
and the constructed name. That routine acquires the world
critical section and the controller critical section at
`+0x8005D0`, publishes the requested seed and name, and
removes world actor entries other than the retained local actor
on the observed transition path. It releases those holds after
publication and the scoped cleanup.

Evidence at S `0x022F4C–0x022F6B` and
`0x025D54–0x025D98`. C evidence is `0x06B8A7–0x06B901`,
dispatch tables `0x06D180` and `0x06D1A4`,
`0x06B999–0x06BA0A`, string `0x301038`, integer stream
import `0x2FC358`, and `0x06F653–0x06F66D`,
`0x06F6E0–0x06F749`, `0x06F7CD–0x06F809`.

### Active-world installation and frame admission

The native preparation worker at C `0x06A8A0` compares the
requested and active identities under the controller
`+0x8005D0` hold. It records whether the seed or name differs.
The mismatch path proceeds through preparation. On the install
path, the worker takes the embedded world's critical section,
reads the then-current requested seed and name, and calls active
setup on that same world object. The setup writes world
`+0x800164` and assigns the name at world `+0x94`.
The normal worker loop has a 20-millisecond sleep and rechecks
its run byte at controller `+0x8005B0`. This sleep belongs to
worker pacing.

Evidence at C `0x06DBA0–0x06DBA3`, `0x06A8E4–0x06A9C3`,
`0x06AF74–0x06AFAB`, `0x1A5319–0x1A532A`,
`0x1A55E4–0x1A5624`, and `0x06B68B–0x06B715`.

The frame takes the world critical section, builds its aggregate
and then compares the active seed with the requested seed. If
they match, it compares the two names by length and byte content.
Only those matching comparisons admit the world calls. The same
seed with a different name, or the same name with a different
seed, skips the world sequence. An admitted elapsed of at most
20 gives one world call with that signed value. Above 20, the
frame makes calls of 20 followed by a final piece in 1 through
20. The zero-byte path can supply zero or negative elapsed.

Either admission failure joins later frame work at C
`0x08D5F0`, ahead of the scoped receipt and presentation paths.
The display driver still commits its sampled clock value after
the frame returns. With ordinary repeated display updates, a
readiness skip therefore consumes that frame's display interval.
The next display difference starts from the new sample. No
elapsed catch-up list or timer was found in this producer and
admission path.

Evidence at C `0x089020–0x08902F`, `0x08C138–0x08C13F`,
`0x080DF0`, `0x0787F0`, `0x051440`, `0x00C520`,
`0x08CE2E–0x08CF49`, `0x08D5F0`, and
`0x0C8633–0x0C86EC`.

## Remaining boundaries

The known independent pacing mechanisms are the optional guarded
world elapsed adjustment, the outer-loop minimum step, and the
preparation worker's normal sleep. The verified readiness skip is
the seed and name comparison inside a running client frame. The
full scheduler behavior when display updates themselves are
withheld, exceptional connection exits, and unresolved activation
of `+0x8006E5` remain outside this coverage.
